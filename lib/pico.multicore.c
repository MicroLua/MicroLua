#include <stdlib.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/multicore.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/main.h"
#include "mlua/util.h"

// TODO: Provide a way to shut down the Lua interpreter running in core1 and
//       correctly release all resources.

static lua_State* interpreters[NUM_CORES - 1];

static void launch_core1(void) {
    lua_State* ls = interpreters[get_core_num() - 1];
    mlua_run_main(ls);
    uint32_t save = spin_lock_blocking(mlua_lock);
    interpreters[get_core_num() - 1] = NULL;
    spin_unlock(mlua_lock, save);
    lua_close(ls);
}

static int mod_launch_core1(lua_State* ls) {
    uint const core = 1;
    if (get_core_num() == core)
        luaL_error(ls, "cannot launch core 1 from itself");
    lua_State* ls1 = mlua_new_interpreter();
    uint32_t save = spin_lock_blocking(mlua_lock);
    bool busy = interpreters[core - 1] != NULL;
    if (!busy) interpreters[core - 1] = ls1;
    spin_unlock(mlua_lock, save);
    if (busy) {
        lua_close(ls1);
        return luaL_error(ls, "core 1 is already running an interpreter");
    }
    size_t len;
    lua_pushlstring(ls1, luaL_checklstring(ls, 1, &len), len);
    lua_pushstring(ls1, luaL_optstring(ls, 2, "main"));
    multicore_launch_core1(&launch_core1);
    return 0;
}

MLUA_FUNC_0_0(mod_, multicore_, reset_core1)

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(reset_core1),
    MLUA_SYM(launch_core1),
#undef MLUA_SYM
};

int luaopen_pico_multicore(lua_State* ls) {
    // Create the module.
    mlua_new_table(ls, module_regs);
    return 1;
}
