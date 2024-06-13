// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include <stdbool.h>
#include <stdint.h>

#include "hardware/i2c.h"

#include "mlua/hardware.i2c.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

// BUG(pico-sdk): This module doesn't use the pico_i2c_slave library, because
// its implementation is flaky (it only works if IRQs are serviced very quickly,
// and there are potential race conditions). Instead, it implements a different
// API that works correctly even with slow handlers.

static int handle_i2c_slave_event_1(lua_State* ls, int status,
                                    lua_KContext ctx);

static int handle_i2c_slave_event(lua_State* ls) {
    return handle_i2c_slave_event_1(ls, LUA_OK, 0);
}

static int handle_i2c_slave_event_1(lua_State* ls, int status,
                                    lua_KContext ctx) {
    i2c_hw_t* hw = i2c_get_hw(mlua_to_I2C(ls, lua_upvalueindex(1)));

    // Handle writes.
    while ((hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_RX_FULL_BITS) != 0) {
        lua_pushvalue(ls, lua_upvalueindex(2));
        lua_pushvalue(ls, lua_upvalueindex(1));
        uint32_t dc = hw->data_cmd;
        lua_pushinteger(ls, dc & 0xff);
        lua_pushboolean(ls, (dc & I2C_IC_DATA_CMD_FIRST_DATA_BYTE_BITS) != 0);
        lua_callk(ls, 3, 0, 0, &handle_i2c_slave_event_1);
    }
    hw_set_bits(&hw->intr_mask, I2C_IC_INTR_MASK_M_RX_FULL_BITS);

    // Handle read requests.
    if ((hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_RD_REQ_BITS) == 0) return 0;
    hw->clr_rd_req;
    hw_set_bits(&hw->intr_mask, I2C_IC_INTR_MASK_M_RD_REQ_BITS);
    lua_pushvalue(ls, lua_upvalueindex(3));
    lua_pushvalue(ls, lua_upvalueindex(1));
    return mlua_callk(ls, 1, 0, mlua_cont_return, 0);
}

static int i2c_slave_handler_done(lua_State* ls) {
    i2c_inst_t* inst = mlua_to_I2C(ls, lua_upvalueindex(1));
    i2c_hw_t* hw = i2c_get_hw(inst);
    i2c_set_slave_mode(inst, false, 0);
    hw_clear_bits(&hw->intr_mask,
        I2C_IC_INTR_MASK_M_RX_FULL_BITS | I2C_IC_INTR_MASK_M_RD_REQ_BITS);
    return 0;
}

static int mod_run(lua_State* ls) {
    i2c_inst_t* inst = mlua_check_I2C(ls, 1);
    uint16_t address = luaL_checkinteger(ls, 2);

    // Configure the I2C instance as a slave.
    i2c_set_slave_mode(inst, true, address);

    // Start the event handler thread.
    lua_pushvalue(ls, 1);  // inst
    lua_pushvalue(ls, 3);  // on_receive
    lua_pushvalue(ls, 4);  // on_request
    lua_pushcclosure(ls, &handle_i2c_slave_event, 3);
    lua_pushvalue(ls, 1);  // inst
    lua_pushcclosure(ls, &i2c_slave_handler_done, 1);
    return mlua_event_handle(ls, &mlua_i2c_state[i2c_hw_index(inst)].event,
                             &mlua_cont_return, 1);
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(run, mod_),
};

MLUA_OPEN_MODULE(pico.i2c_slave) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
