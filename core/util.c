#include "mlua/util.h"

void mlua_require(lua_State* ls, char const* module, bool keep) {
    lua_getglobal(ls, "require");
    lua_pushstring(ls, module);
    lua_call(ls, 1, keep ? 1 : 0);
}

bool mlua_to_cbool(lua_State* ls, int arg) {
    if (lua_isinteger(ls, arg)) return lua_tointeger(ls, arg) != 0;
    if (lua_type(ls, arg) == LUA_TNUMBER)
        return lua_tonumber(ls, arg) != l_mathop(0.0);
    return lua_toboolean(ls, arg);
}

spin_lock_t* mlua_lock;

static __attribute__((constructor)) void init_mlua_lock(void) {
    mlua_lock = spin_lock_instance(next_striped_spin_lock_num());
}

void mlua_reg_push_boolean(lua_State* ls, MLuaReg const* reg, int nup) {
    lua_pushboolean(ls, reg->boolean);
}

void mlua_reg_push_integer(lua_State* ls, MLuaReg const* reg, int nup) {
    lua_pushinteger(ls, reg->integer);
}

void mlua_reg_push_number(lua_State* ls, MLuaReg const* reg, int nup) {
    lua_pushnumber(ls, reg->number);
}

void mlua_reg_push_string(lua_State* ls, MLuaReg const* reg, int nup) {
    lua_pushstring(ls, reg->string);
}

void mlua_reg_push_function(lua_State* ls, MLuaReg const* reg, int nup) {
    for (int i = 0; i < nup; ++i) lua_pushvalue(ls, -nup);
    lua_pushcclosure(ls, reg->function, nup);
}

void mlua_set_fields(lua_State* ls, MLuaReg const* fields, int nup) {
    luaL_checkstack(ls, nup, "too many upvalues");
    for (; fields->name != NULL; ++fields) {
        fields->push(ls, fields, nup);
        lua_setfield(ls, -(nup + 2), fields->name);
    }
    lua_pop(ls, nup);  // Remove upvalues
}

void mlua_new_class(lua_State* ls, char const* name, MLuaReg const* fields) {
    luaL_newmetatable(ls, name);
    mlua_set_fields(ls, fields, 0);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, -2, "__index");
}
