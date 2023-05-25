#include <stdlib.h>
#include <string.h>

#include "pico/multicore.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/main.h"
#include "mlua/util.h"

// TODO: Provide a way to shut down the Lua interpreter running in core1 and
//       correctly release all resources.

static struct mlua_entry_point entry_points[NUM_CORES - 1];

static void launch_core1(void) {
    struct mlua_entry_point* ep = &entry_points[get_core_num() - 1];
    mlua_main(ep);
    free((char*)ep->func);
    free((char*)ep->module);
    ep->module = NULL;
    ep->func = NULL;
}

static int mod_launch_core1(lua_State* ls) {
    int core = get_core_num();
    if (core == 1) luaL_error(ls, "cannot launch core %d from itself", core);
    struct mlua_entry_point* ep = &entry_points[1 - 1];
    ep->module = strdup(luaL_checkstring(ls, 1));
    ep->func = strdup(luaL_optstring(ls, 2, "main"));
    multicore_launch_core1(&launch_core1);
    return 0;
}

MLUA_FUNC_0_0(mod_, multicore_, reset_core1)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(reset_core1),
    X(launch_core1),
#undef X
    {NULL},
};

int luaopen_pico_multicore(lua_State* ls) {
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
