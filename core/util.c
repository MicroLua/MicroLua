#include "mlua/util.h"

#include "pico/platform.h"

spin_lock_t* mlua_lock;

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

void* mlua_check_userdata(lua_State* ls, int arg) {
    void* ud = lua_touserdata(ls, arg);
    luaL_argexpected(ls, ud != NULL, arg, "userdata");
    return ud;
}

void* mlua_check_userdata_or_nil(lua_State* ls, int arg) {
    if (lua_isnoneornil(ls, arg)) return NULL;
    void* ud = lua_touserdata(ls, arg);
    luaL_argexpected(ls, ud != NULL, arg, "userdata or nil");
    return ud;
}

int mlua_index_undefined(lua_State* ls) {
    return luaL_error(ls, "undefined symbol: %s", lua_tostring(ls, 2));
}

void mlua_sym_push_boolean(lua_State* ls, MLuaSym const* sym) {
    lua_pushboolean(ls, sym->boolean);
}

void mlua_sym_push_integer(lua_State* ls, MLuaSym const* sym) {
    lua_pushinteger(ls, sym->integer);
}

void mlua_sym_push_number(lua_State* ls, MLuaSym const* sym) {
    lua_pushnumber(ls, sym->number);
}

void mlua_sym_push_string(lua_State* ls, MLuaSym const* sym) {
    lua_pushstring(ls, sym->string);
}

void mlua_sym_push_function(lua_State* ls, MLuaSym const* sym) {
    lua_pushcfunction(ls, sym->function);
}

void mlua_sym_push_lightuserdata(lua_State* ls, MLuaSym const* sym) {
    lua_pushlightuserdata(ls, sym->lightuserdata);
}

void mlua_set_fields_(lua_State* ls, MLuaSym const* fields, int cnt) {
    for (; cnt > 0; --cnt, ++fields) {
        fields->push(ls, fields);
        char const* name = fields->name;
        if (name[0] == '@' && name[1] == '_') name += 2;
        lua_setfield(ls, -2, name);
    }
}

void mlua_new_table_(lua_State* ls, MLuaSym const* fields, int narr, int nrec) {
    lua_createtable(ls, narr, nrec);
    mlua_set_fields_(ls, fields, nrec);
}

static char const Strict_name[] = "mlua.Strict";

static MLuaSym const Strict_syms[] = {
    MLUA_SYM_V(__index, function, &mlua_index_undefined),
};

void mlua_new_module_(lua_State* ls, MLuaSym const* fields, int narr,
                      int nrec) {
    mlua_new_table_(ls, fields, narr, nrec);
    luaL_getmetatable(ls, Strict_name);
    lua_setmetatable(ls, -2);
}

void mlua_new_class_(lua_State* ls, char const* name, MLuaSym const* fields,
                     int cnt) {
    luaL_newmetatable(ls, name);
    mlua_set_fields_(ls, fields, cnt);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, -2, "__index");
    luaL_getmetatable(ls, Strict_name);
    lua_setmetatable(ls, -2);
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

static int Function___close_1(lua_State* ls, int status, lua_KContext ctx) {
    return 0;
}

static int Function___close(lua_State* ls) {
    // Call the function itself, passing through the remaining arguments. This
    // makes to-be-closed functions the equivalent of deferreds.
    lua_callk(ls, lua_gettop(ls) - 1, 0, 0, &Function___close_1);
    return Function___close_1(ls, LUA_OK, 0);
}

int mlua_cont_return_ctx(lua_State* ls, int status, lua_KContext ctx) {
    return ctx;
}

int mlua_thread_meta(lua_State* ls, char const* name) {
    lua_pushthread(ls);
    int res = luaL_getmetafield(ls, -1, name);
    lua_remove(ls, res != LUA_TNIL ? -2 : -1);
    return res;
}

void mlua_thread_start(lua_State* ls) {
    lua_pushthread(ls);
    if (luaL_getmetafield(ls, -1, "start") == LUA_TNIL) {
        luaL_error(ls, "threads have no start method");
        return;
    }
    lua_rotate(ls, -3, 1);
    lua_pop(ls, 1);
    lua_call(ls, 1, 1);
}

void mlua_thread_kill(lua_State* ls) {
    if (luaL_getmetafield(ls, -1, "kill") == LUA_TNIL) {
        luaL_error(ls, "threads have no kill method");
        return;
    }
    lua_rotate(ls, -2, 1);
    lua_call(ls, 1, 0);
}

bool mlua_thread_is_alive(lua_State* thread) {
    switch (lua_status(thread)) {
    case LUA_YIELD:
        return true;
    case LUA_OK: {
        lua_Debug ar;
        return lua_getstack(thread, 0, &ar) || lua_gettop(thread) != 0;
    }
    default:
        return false;
    }
}

static __attribute__((constructor)) void init(void) {
    mlua_lock = spin_lock_instance(next_striped_spin_lock_num());
#if LIB_MLUA_MOD_MLUA_EVENT
    for (uint core = 0; core < NUM_CORES; ++core) yield_enabled[core] = true;
#endif
}

void mlua_util_init(lua_State* ls) {
    // Create the Strict metatable, and set it on _G.
    lua_pushglobaltable(ls);
    luaL_newmetatable(ls, Strict_name);
    mlua_set_fields(ls, Strict_syms);
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Set globals.
    lua_pushstring(ls, LUA_RELEASE);
    lua_setglobal(ls, "_RELEASE");
    lua_pushcfunction(ls, &global_yield_enabled);
    lua_setglobal(ls, "yield_enabled");

    // Set a metatable on functions.
    lua_pushcfunction(ls, &Function___close);  // Any function will do
    lua_createtable(ls, 0, 1);
    lua_pushcfunction(ls, &Function___close);
    lua_setfield(ls, -2, "__close");
    lua_setmetatable(ls, -2);
    lua_pop(ls, 1);

    // Load the mlua module and register it in globals.
    static char const mlua_name[] = "mlua";
    mlua_require(ls, mlua_name, true);
    lua_setglobal(ls, mlua_name);
}
