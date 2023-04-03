#include "mlua/util.h"

void mlua_reg_push_bool(lua_State* ls, mlua_reg const* reg, int nup) {
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

void mlua_set_funcs(lua_State* ls, mlua_reg const* reg, int nup) {
    luaL_checkstack(ls, nup, "too many upvalues");
    for (; reg->name != NULL; ++reg) {
        reg->push(ls, reg, nup);
        lua_setfield(ls, -(nup + 2), reg->name);
    }
    lua_pop(ls, nup);  // Remove upvalues
}
