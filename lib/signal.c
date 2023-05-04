#include "mlua/signal.h"

#include <stdint.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/platform.h"

#include "mlua/util.h"

#include "pico/time.h"

#define NUM_SIGNALS 128
#define SIGNALS_SIZE ((NUM_SIGNALS + 31) / 32)

struct Signals {
    uint32_t pending[SIGNALS_SIZE];
    uint32_t mask[SIGNALS_SIZE];
    bool wake;
};

static struct Signals signals[NUM_CORES];

void set_handler(lua_State* ls, struct Signals* sigs, int sig, int handler) {
    lua_rawgetp(ls, LUA_REGISTRYINDEX, sigs);
    lua_pushinteger(ls, sig);
    lua_pushvalue(ls, handler);
    lua_settable(ls, -3);
    lua_pop(ls, 1);
}

int mlua_signal_claim(lua_State* ls, int handler) {
    struct Signals* sigs = &signals[get_core_num()];
    for (int i = 0; i < SIGNALS_SIZE; ++i) {
        int idx = __builtin_ffs(~sigs->mask[i]);
        if (idx > 0) {
            --idx;
            sigs->mask[i] |= 1u << idx;
            int sig = i * 32 + idx;
            set_handler(ls, sigs, sig, handler);
            return sig;
        }
    }
    return luaL_error(ls, "no signals available");
}

void mlua_signal_unclaim(lua_State* ls, int sig) {
    if (sig < 0) return;
    struct Signals* sigs = &signals[get_core_num()];
    lua_rawgetp(ls, LUA_REGISTRYINDEX, sigs);
    lua_pushinteger(ls, sig);
    lua_pushnil(ls);
    lua_settable(ls, -3);
    lua_pop(ls, 1);
    sigs->mask[sig / 32] &= ~(1u << (sig % 32));
}

void mlua_signal_set_handler(lua_State* ls, int sig, int handler) {
    if (sig < 0) return;
    struct Signals* sigs = &signals[get_core_num()];
    if ((sigs->mask[sig / 32] & (1u << (sig % 32))) == 0) return;
    set_handler(ls, sigs, sig, handler);
}

void __time_critical_func(mlua_signal_set)(int sig, bool pending) {
    if (sig < 0) return;
    struct Signals* sigs = &signals[get_core_num()];
    int index = sig / 32;
    uint32_t mask = 1u << (sig % 32);
    uint32_t save = save_and_disable_interrupts();
    if (pending) {
        sigs->pending[index] |= mask;
    } else {
        sigs->pending[index] &= ~mask;
    }
    restore_interrupts(save);
}

static int mod_dispatch(lua_State* ls) {
    struct Signals* sigs = &signals[get_core_num()];
    sigs->wake = !lua_toboolean(ls, 1);
    lua_rawgetp(ls, LUA_REGISTRYINDEX, sigs);
    uint32_t active[SIGNALS_SIZE];
    for (;;) {
        // Check for active signals.
        int found = SIGNALS_SIZE;
        for (;;) {
            uint32_t save = save_and_disable_interrupts();
            for (int i = SIGNALS_SIZE - 1; i >= 0; --i) {
                uint32_t a = sigs->pending[i] & sigs->mask[i];
                active[i] = a;
                if (a != 0) found = i;
                sigs->pending[i] = 0;
            }
            if (sigs->wake || found < SIGNALS_SIZE) {
                restore_interrupts(save);
                break;
            }
            __wfi();  // TODO: Use WFE with SEVONPEND?
            restore_interrupts(save);
        }

        // Call handlers for active signals.
        for (int i = found; i < SIGNALS_SIZE; ++i) {
            uint32_t a = active[i];
            for (;;) {
                int idx = __builtin_ffs(a);
                if (idx == 0) break;
                --idx;
                a &= ~(1u << idx);
                lua_pushinteger(ls, i * 32 + idx);
                lua_gettable(ls, -2);
                lua_call(ls, 0, 0);
            }
        }
        if (sigs->wake) break;
    }
    return 0;
}

static int mod_wake(lua_State* ls) {
    struct Signals* sigs = &signals[get_core_num()];
    sigs->wake = true;
    return 0;
}

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(dispatch),
    X(wake),
#undef X
    {NULL},
};

int luaopen_signal(lua_State* ls) {
    // Initialize internal state.
    struct Signals* sigs = &signals[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    memset(sigs, 0, sizeof(*sigs));
    restore_interrupts(save);

    lua_newtable(ls);  // Signal handler table
    lua_rawsetp(ls, LUA_REGISTRYINDEX, sigs);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
