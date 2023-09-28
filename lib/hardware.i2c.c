#include <stdbool.h>
#include <stdint.h>

#include "hardware/i2c.h"

#include "mlua/event.h"
#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/util.h"

static char const I2C_name[] = "hardware.i2c.I2C";

static i2c_inst_t** new_I2C(lua_State* ls) {
    i2c_inst_t** v = lua_newuserdatauv(ls, sizeof(i2c_inst_t*), 0);
    luaL_getmetatable(ls, I2C_name);
    lua_setmetatable(ls, -2);
    return v;
}

static inline i2c_inst_t* check_I2C(lua_State* ls, int arg) {
    return *((i2c_inst_t**)luaL_checkudata(ls, arg, I2C_name));
}

static inline i2c_inst_t* to_I2C(lua_State* ls, int arg) {
    return *((i2c_inst_t**)lua_touserdata(ls, arg));
}

static int I2C_regs_base(lua_State* ls) {
    lua_pushinteger(ls, (uintptr_t)i2c_get_hw(check_I2C(ls, 1)));
    return 1;
}

static int I2C_write_blocking(lua_State* ls) {
    i2c_inst_t* inst = check_I2C(ls, 1);
    uint16_t addr = luaL_checkinteger(ls, 2);
    size_t len;
    uint8_t const* src = (uint8_t const*)luaL_checklstring(ls, 3, &len);
    bool nostop = mlua_to_cbool(ls, 4);

    lua_pushinteger(ls, i2c_write_blocking(inst, addr, src, len, nostop));
    return 1;
}

static int I2C_read_blocking(lua_State* ls) {
    i2c_inst_t* inst = check_I2C(ls, 1);
    uint16_t addr = luaL_checkinteger(ls, 2);
    size_t len = luaL_checkinteger(ls, 3);
    bool nostop = mlua_to_cbool(ls, 4);

    luaL_Buffer buf;
    uint8_t* dst = (uint8_t*)luaL_buffinitsize(ls, &buf, len);
    int count = i2c_read_blocking(inst, addr, dst, len, nostop);
    if (count >= 0) {
        luaL_pushresultsize(&buf, count);
    } else {
        lua_pushinteger(ls, count);
    }
    return 1;
}

static int I2C_read_data_cmd(lua_State* ls) {
    i2c_inst_t* inst = check_I2C(ls, 1);
    i2c_hw_t* hw = i2c_get_hw(inst);
    lua_pushinteger(ls, hw->data_cmd);
    return 1;
}

MLUA_FUNC_1_2(I2C_, i2c_, init, lua_pushinteger, check_I2C, luaL_checkinteger)
MLUA_FUNC_0_1(I2C_, i2c_, deinit, check_I2C)
MLUA_FUNC_1_2(I2C_, i2c_, set_baudrate, lua_pushinteger, check_I2C,
              luaL_checkinteger)
MLUA_FUNC_0_3(I2C_, i2c_, set_slave_mode, check_I2C, mlua_to_cbool,
              luaL_checkinteger)
MLUA_FUNC_1_1(I2C_, i2c_, hw_index, lua_pushinteger, check_I2C)
MLUA_FUNC_1_1(I2C_, i2c_, get_write_available, lua_pushinteger, check_I2C)
MLUA_FUNC_1_1(I2C_, i2c_, get_read_available, lua_pushinteger, check_I2C)
MLUA_FUNC_1_1(I2C_, i2c_, read_byte_raw, lua_pushinteger, check_I2C)
MLUA_FUNC_0_2(I2C_, i2c_, write_byte_raw, check_I2C, luaL_checkinteger)
MLUA_FUNC_1_2(I2C_, i2c_, get_dreq, lua_pushinteger, check_I2C, mlua_to_cbool)

MLUA_SYMBOLS(I2C_syms) = {
    MLUA_SYM_F(init, I2C_),
    MLUA_SYM_F(deinit, I2C_),
    MLUA_SYM_F(set_baudrate, I2C_),
    MLUA_SYM_F(set_slave_mode, I2C_),
    MLUA_SYM_F(hw_index, I2C_),
    MLUA_SYM_F(regs_base, I2C_),
    // TODO: MLUA_SYM_F(write_blocking_until, I2C_),
    // TODO: MLUA_SYM_F(read_blocking_until, I2C_),
    // TODO: MLUA_SYM_F(write_timeout_us, I2C_),
    // TODO: MLUA_SYM_F(write_timeout_per_char_us, I2C_),
    // TODO: MLUA_SYM_F(read_timeout_us, I2C_),
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
};

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(NUM, integer, NUM_I2CS),
    MLUA_SYM_V(_default, boolean, false),
};

MLUA_OPEN_MODULE(hardware.i2c) {
    mlua_event_require(ls);
    mlua_require(ls, "mlua.int64", false);

    // Create the module.
    mlua_new_module(ls, NUM_I2CS, module_syms);
    int mod_index = lua_gettop(ls);

    // Create the I2C class.
    mlua_new_class(ls, I2C_name, I2C_syms, true);
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
