#include "mlua/util.h"

bool mlua_to_cbool(lua_State* ls, int arg) {
    if (lua_isinteger(ls, arg)) return lua_tointeger(ls, arg) != 0;
    if (lua_type(ls, arg) == LUA_TNUMBER)
        lua_tonumber(ls, arg) != l_mathop(0.0);
    return lua_toboolean(ls, arg);
}

void mlua_reg_push_boolean(lua_State* ls, mlua_reg const* reg, int nup) {
    lua_pushboolean(ls, reg->boolean);
}

void mlua_reg_push_integer(lua_State* ls, mlua_reg const* reg, int nup) {
    lua_pushinteger(ls, reg->integer);
}

void mlua_reg_push_number(lua_State* ls, mlua_reg const* reg, int nup) {
    lua_pushnumber(ls, reg->number);
}

void mlua_reg_push_string(lua_State* ls, mlua_reg const* reg, int nup) {
    lua_pushstring(ls, reg->string);
}

void mlua_reg_push_function(lua_State* ls, mlua_reg const* reg, int nup) {
    for (int i = 0; i < nup; ++i) lua_pushvalue(ls, -nup);
    lua_pushcclosure(ls, reg->function, nup);
}

void mlua_set_fields(lua_State* ls, mlua_reg const* reg, int nup) {
    luaL_checkstack(ls, nup, "too many upvalues");
    for (; reg->name != NULL; ++reg) {
        reg->push(ls, reg, nup);
        lua_setfield(ls, -(nup + 2), reg->name);
    }
    lua_pop(ls, nup);  // Remove upvalues
}
