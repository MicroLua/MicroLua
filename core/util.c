#include "mlua/util.h"

#include "pico/platform.h"

void mlua_require(lua_State* ls, char const* module, bool keep) {
    lua_getglobal(ls, "require");
    lua_pushstring(ls, module);
    lua_call(ls, 1, keep ? 1 : 0);
}

bool mlua_to_cbool(lua_State* ls, int index) {
    if (lua_isinteger(ls, index)) return lua_tointeger(ls, index) != 0;
    if (lua_type(ls, index) == LUA_TNUMBER)
        return lua_tonumber(ls, index) != l_mathop(0.0);
    return lua_toboolean(ls, index);
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

#if LIB_MLUA_MOD_MLUA_EVENT

static bool yield_enabled[NUM_CORES];

bool mlua_yield_enabled(void) { return yield_enabled[get_core_num()]; }

#endif  // LIB_MLUA_MOD_MLUA_EVENT

static int global_yield_enabled(lua_State* ls) {
#if LIB_MLUA_MOD_MLUA_EVENT
    bool* en = &yield_enabled[get_core_num()];
    lua_pushboolean(ls, *en);
    if (!lua_isnoneornil(ls, 1)) *en = mlua_to_cbool(ls, 1);
#else
    lua_pushboolean(ls, false);
#endif
    return 1;
}

static int Function___close(lua_State* ls) {
    // Call the function itself, passing through the remaining arguments. This
    // makes to-be-closed functions the equivalent of deferreds.
    lua_call(ls, lua_gettop(ls) - 1, 0);
    return 0;
}

static __attribute__((constructor)) void init(void) {
#if LIB_MLUA_MOD_MLUA_EVENT
    for (uint core = 0; core < NUM_CORES; ++core) yield_enabled[core] = true;
#endif
}

void mlua_util_init(lua_State* ls) {
    // Set globals.
    lua_pushcfunction(ls, &global_yield_enabled);
    lua_setglobal(ls, "yield_enabled");

    // Set a metatable on functions.
    lua_pushcfunction(ls, &Function___close);  // Any function will do
    lua_createtable(ls, 0, 1);
    lua_pushcfunction(ls, &Function___close);
    lua_setfield(ls, -2, "__close");
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);
}
