// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/hardware.i2c.h"

#include <stdbool.h>
#include <stdint.h>

#include "pico.h"
#include "pico/time.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

// TODO: Accept and return mutable buffers when writing and reading

char const mlua_I2C_name[] = "hardware.i2c.I2C";

static i2c_inst_t** new_I2C(lua_State* ls) {
    i2c_inst_t** v = lua_newuserdatauv(ls, sizeof(i2c_inst_t*), 0);
    luaL_getmetatable(ls, mlua_I2C_name);
    lua_setmetatable(ls, -2);
    return v;
}

static int I2C_regs(lua_State* ls) {
    return lua_pushlightuserdata(ls, i2c_get_hw(mlua_check_I2C(ls, 1))), 1;
}

#if LIB_MLUA_MOD_MLUA_THREAD

MLuaI2CState mlua_i2c_state[NUM_I2CS];

static void __time_critical_func(handle_i2c_irq)(void) {
    uint num = __get_current_exception() - VTABLE_FIRST_IRQ - I2C0_IRQ;
    i2c_hw_t* hw = i2c_get_hw(i2c_get_instance(num));
    hw->intr_mask = 0;
    mlua_event_set(&mlua_i2c_state[num].event);
}

static int I2C_enable_irq(lua_State* ls) {
    i2c_inst_t* inst = mlua_check_I2C(ls, 1);
    uint num = i2c_hw_index(inst);
    uint irq = I2C0_IRQ + num;
    MLuaI2CState* state = &mlua_i2c_state[num];
    if (!mlua_event_enable_irq(ls, &state->event, irq,
                               &handle_i2c_irq, 2, -1)) {
        return luaL_error(ls, "I2C%d: IRQ already enabled", num);
    }
    return 0;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static int I2C_deinit(lua_State* ls) {
    i2c_inst_t* inst = mlua_check_I2C(ls, 1);
#if LIB_MLUA_MOD_MLUA_THREAD
    lua_pushcfunction(ls, &I2C_enable_irq);
    lua_pushvalue(ls, 1);
    lua_pushboolean(ls, false);
    lua_call(ls, 2, 0);
#endif
    i2c_deinit(inst);
    return 0;
}

static int write_loop(lua_State* ls, bool timeout) {
    if (timeout) {
        luaL_pushfail(ls);
        lua_pushinteger(ls, PICO_ERROR_TIMEOUT);
        return 2;
    }
    i2c_inst_t* inst = mlua_to_I2C(ls, 1);
    i2c_hw_t* hw = i2c_get_hw(inst);
    uint32_t abort_reason = lua_tointeger(ls, 6);
    size_t offset = lua_tointeger(ls, 7);
    lua_pop(ls, 1);

    // Initialize the transfer.
    if (offset == (size_t)-1) {
        hw->enable = 0;
        hw->tar = lua_tointeger(ls, 2);  // addr
        hw->enable = 1;
        offset = 0;
    }

    // Send data.
    bool nostop = mlua_to_cbool(ls, 4);
    if (abort_reason == 0) {
        size_t len;
        uint8_t const* src = (uint8_t const*)lua_tolstring(ls, 3, &len);
        while (offset < len) {
            abort_reason = hw->tx_abrt_source;
            if (abort_reason != 0) {  // Tx was aborted, stop sending
                hw->clr_tx_abrt;
                break;
            }
            if (i2c_get_write_available(inst) == 0) {
                // Tx FIFO is full, suspend until it's empty.
                hw_set_bits(&hw->intr_mask,
                    I2C_IC_INTR_MASK_M_TX_EMPTY_BITS
                    | I2C_IC_INTR_MASK_M_TX_ABRT_BITS);
                lua_pushinteger(ls, offset);
                return -1;
            }
            hw->data_cmd = bool_to_bit(offset == 0 && inst->restart_on_next)
                               << I2C_IC_DATA_CMD_RESTART_LSB
                           | bool_to_bit(offset == len - 1 && !nostop)
                               << I2C_IC_DATA_CMD_STOP_LSB
                           | src[offset];
            ++offset;
        }
    }

    // Wait for the STOP condition.
    if (abort_reason != 0 || !nostop) {
        if ((hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS) == 0) {
            // STOP condition not detected yet, suspend until it is.
            hw_set_bits(&hw->intr_mask, I2C_IC_INTR_MASK_M_STOP_DET_BITS);
            lua_pop(ls, 1);
            lua_pushinteger(ls, abort_reason);
            lua_pushinteger(ls, offset);
            return -1;
        }
        hw->clr_stop_det;
    }
    inst->restart_on_next = nostop;

    // Compute the call result.
    if ((abort_reason & (I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS |
                         I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS)) != 0) {
        luaL_pushfail(ls);
        lua_pushinteger(ls, PICO_ERROR_GENERIC);
        return 2;
    }
    lua_pushinteger(ls, offset);
    return 1;
}

static int I2C_write_blocking(lua_State* ls) {
    i2c_inst_t* inst = mlua_check_I2C(ls, 1);
    uint16_t addr = luaL_checkinteger(ls, 2);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 3, &len);
    bool nostop = mlua_to_cbool(ls, 4);

    MLuaEvent* event = &mlua_i2c_state[i2c_hw_index(inst)].event;
    if (mlua_event_can_wait(ls, event, 0)) {
        lua_settop(ls, 5);
        lua_pushinteger(ls, 0);  // abort_reason
        lua_pushinteger(ls, -1);  // offset
        return mlua_event_wait(ls, event, 0, &write_loop, 5);
    }

    int res;
    if (lua_isnoneornil(ls, 5)) {
        res = i2c_write_blocking(inst, addr, src, len, nostop);
    } else {
        res = i2c_write_blocking_until(
            inst, addr, src, len, nostop,
            from_us_since_boot(mlua_check_time(ls, 5)));
    }
    if (res < 0) {
        luaL_pushfail(ls);
        lua_pushinteger(ls, res);
        return 2;
    }
    lua_pushinteger(ls, res);
    return 1;
}

static int I2C_write_timeout_us(lua_State* ls) {
    uint64_t timeout = mlua_check_int64(ls, 5);
    lua_settop(ls, 4);
    mlua_push_deadline(ls, timeout);
    return I2C_write_blocking(ls);
}

static int read_loop(lua_State* ls, bool timeout) {
    if (timeout) {
        lua_settop(ls, 0);
        luaL_pushfail(ls);
        lua_pushinteger(ls, PICO_ERROR_TIMEOUT);
        return 2;
    }
    i2c_inst_t* inst = mlua_to_I2C(ls, 1);
    i2c_hw_t* hw = i2c_get_hw(inst);
    size_t wcnt = lua_tointeger(ls, 6);
    size_t offset = lua_tointeger(ls, 7);

    // Initialize the transfer.
    if (offset == (size_t)-1) {
        hw->enable = 0;
        hw->tar = lua_tointeger(ls, 2);  // addr
        hw->enable = 1;
        offset = 0;
    }

    // Receive data.
    uint32_t abort_reason = 0;
    bool nostop = mlua_to_cbool(ls, 4);
    size_t len = lua_tointeger(ls, 3);
    while (offset < len) {
        // Check for an aborted transaction.
        abort_reason = hw->tx_abrt_source;
        if (abort_reason != 0) {
            hw->clr_tx_abrt;
            break;
        }

        // Check if there is work to do, and suspend if not.
        size_t rend = offset + i2c_get_read_available(inst);
        if (rend > len) rend = len;
        size_t wend = wcnt + i2c_get_write_available(inst);
        if (wend > len) wend = len;
        if (offset >= rend && wcnt >= wend) {
            hw_set_bits(&hw->intr_mask,
                bool_to_bit(wcnt < len) << I2C_IC_INTR_MASK_M_TX_EMPTY_LSB
                | I2C_IC_INTR_MASK_M_RX_FULL_BITS
                | I2C_IC_INTR_MASK_M_TX_ABRT_BITS);
            lua_pushinteger(ls, wcnt);
            lua_replace(ls, 6);
            lua_pushinteger(ls, offset);
            lua_replace(ls, 7);
            return -1;
        }

        // Read received data.
        if (offset < rend) {
            if (lua_gettop(ls) >= LUA_MINSTACK - 2) {
                lua_concat(ls, lua_gettop(ls) - 7);
            }
            size_t cnt = rend - offset;
            luaL_Buffer buf;
            uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, cnt);
            uint8_t* end = dst + cnt;
            while (dst != end) *dst++ = hw->data_cmd;
            luaL_pushresultsize(&buf, cnt);
            offset = rend;
        }

        // Fill the Tx FIFO.
        while (wcnt < wend) {
            hw->data_cmd = bool_to_bit(wcnt == 0 && inst->restart_on_next)
                               << I2C_IC_DATA_CMD_RESTART_LSB
                           | bool_to_bit(wcnt == len - 1 && !nostop)
                               << I2C_IC_DATA_CMD_STOP_LSB
                           | I2C_IC_DATA_CMD_CMD_BITS;
            ++wcnt;
        }
    }
    inst->restart_on_next = nostop;

    // Compute the call result.
    if ((abort_reason & (I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS |
                         I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS)) != 0) {
        lua_settop(ls, 0);
        luaL_pushfail(ls);
        lua_pushinteger(ls, PICO_ERROR_GENERIC);
        return 2;
    }
    lua_concat(ls, lua_gettop(ls) - 7);
    return 1;
}

static int I2C_read_blocking(lua_State* ls) {
    i2c_inst_t* inst = mlua_check_I2C(ls, 1);
    uint16_t addr = luaL_checkinteger(ls, 2);
    size_t len = luaL_checkinteger(ls, 3);
    bool nostop = mlua_to_cbool(ls, 4);

    MLuaEvent* event = &mlua_i2c_state[i2c_hw_index(inst)].event;
    if (mlua_event_can_wait(ls, event, 0)) {
        lua_settop(ls, 5);
        lua_pushinteger(ls, 0);  // wcnt
        lua_pushinteger(ls, -1);  // offset
        return mlua_event_wait(ls, event, 0, &read_loop, 5);
    }

    bool no_deadline = lua_isnoneornil(ls, 5);
    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    int count;
    if (no_deadline) {
        count = i2c_read_blocking(inst, addr, dst, len, nostop);
    } else {
        count = i2c_read_blocking_until(
            inst, addr, dst, len, nostop,
            from_us_since_boot(mlua_check_time(ls, 5)));
    }
    if (count < 0) {
        luaL_pushfail(ls);
        lua_pushinteger(ls, count);
        return 2;
    }
    luaL_pushresultsize(&buf, count);
    return 1;
}

static int I2C_read_timeout_us(lua_State* ls) {
    uint64_t timeout = mlua_check_int64(ls, 5);
    lua_settop(ls, 4);
    mlua_push_deadline(ls, timeout);
    return I2C_read_blocking(ls);
}

static int I2C_read_data_cmd(lua_State* ls) {
    i2c_inst_t* inst = mlua_check_I2C(ls, 1);
    i2c_hw_t* hw = i2c_get_hw(inst);
    lua_pushinteger(ls, hw->data_cmd);
    return 1;
}

MLUA_FUNC_R2(I2C_, i2c_, init, lua_pushinteger, mlua_check_I2C,
             luaL_checkinteger)
MLUA_FUNC_R2(I2C_, i2c_, set_baudrate, lua_pushinteger, mlua_check_I2C,
             luaL_checkinteger)
MLUA_FUNC_V3(I2C_, i2c_, set_slave_mode, mlua_check_I2C, mlua_to_cbool,
             luaL_checkinteger)
MLUA_FUNC_R1(I2C_, i2c_, hw_index, lua_pushinteger, mlua_check_I2C)
MLUA_FUNC_R1(I2C_, i2c_, get_write_available, lua_pushinteger, mlua_check_I2C)
MLUA_FUNC_R1(I2C_, i2c_, get_read_available, lua_pushinteger, mlua_check_I2C)
MLUA_FUNC_R1(I2C_, i2c_, read_byte_raw, lua_pushinteger, mlua_check_I2C)
MLUA_FUNC_V2(I2C_, i2c_, write_byte_raw, mlua_check_I2C, luaL_checkinteger)
MLUA_FUNC_R2(I2C_, i2c_, get_dreq, lua_pushinteger, mlua_check_I2C,
             mlua_to_cbool)

#define I2C_write_blocking_until I2C_write_blocking
#define I2C_read_blocking_until I2C_read_blocking

MLUA_SYMBOLS(I2C_syms) = {
    MLUA_SYM_F(init, I2C_),
    MLUA_SYM_F(deinit, I2C_),
    MLUA_SYM_F(set_baudrate, I2C_),
    MLUA_SYM_F(set_slave_mode, I2C_),
    MLUA_SYM_F(hw_index, I2C_),
    MLUA_SYM_F(regs, I2C_),
    MLUA_SYM_F(write_blocking_until, I2C_),
    MLUA_SYM_F(read_blocking_until, I2C_),
    MLUA_SYM_F(write_timeout_us, I2C_),
    // TODO: MLUA_SYM_F(write_timeout_per_char_us, I2C_),
    MLUA_SYM_F(read_timeout_us, I2C_),
    // TODO: MLUA_SYM_F(read_timeout_per_char_us, I2C_),
    MLUA_SYM_F(write_blocking, I2C_),
    MLUA_SYM_F(read_blocking, I2C_),
    MLUA_SYM_F(get_write_available, I2C_),
    MLUA_SYM_F(get_read_available, I2C_),
    // TODO: MLUA_SYM_F(write_raw_blocking, I2C_),
    // TODO: MLUA_SYM_F(read_raw_blocking, I2C_),
    MLUA_SYM_F(read_data_cmd, I2C_),
    MLUA_SYM_F(read_byte_raw, I2C_),
    MLUA_SYM_F(write_byte_raw, I2C_),
    MLUA_SYM_F(get_dreq, I2C_),
    MLUA_SYM_F_THREAD(enable_irq, I2C_),
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM, integer, NUM_I2CS),
    MLUA_SYM_V(_default, boolean, false),
};

MLUA_OPEN_MODULE(hardware.i2c) {
    mlua_thread_require(ls);
    mlua_require(ls, "mlua.int64", false);

    // Create the module.
    mlua_new_module(ls, NUM_I2CS, module_syms);
    int mod_index = lua_gettop(ls);

    // Create the I2C class.
    mlua_new_class(ls, mlua_I2C_name, I2C_syms, mlua_nosyms);
    lua_pop(ls, 1);

    // Create I2C instances.
    for (uint i = 0; i < NUM_I2CS; ++i) {
        i2c_inst_t* inst = i2c_get_instance(i);
        *new_I2C(ls) = inst;
#ifdef uart_default
        if (inst == i2c_default) {
            lua_pushvalue(ls, -1);
            lua_setfield(ls, mod_index, "default");
        }
#endif
        lua_seti(ls, mod_index, i);
    }
    return 1;
}
