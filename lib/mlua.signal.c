#include "mlua/signal.h"

#include <stdint.h>
#include <string.h>

#include "hardware/sync.h"
#include "pico/platform.h"
#include "pico/time.h"

#include "mlua/int64.h"
#include "mlua/util.h"

#define NUM_SIGNALS 128
#define SIGNALS_SIZE ((NUM_SIGNALS + 31) / 32)

struct Signals {
    uint32_t pending[SIGNALS_SIZE];
    uint32_t mask[SIGNALS_SIZE];
    bool wake;
};

static struct Signals signals[NUM_CORES];

static void set_handler(lua_State* ls, struct Signals* sigs, SigNum sig,
                        int handler) {
    handler = lua_absindex(ls, handler);
    lua_rawgetp(ls, LUA_REGISTRYINDEX, sigs);
    lua_pushinteger(ls, sig);
    lua_pushvalue(ls, handler);
    lua_settable(ls, -3);
    lua_pop(ls, 1);
}

void mlua_signal_claim(lua_State* ls, SigNum* psig, int handler) {
    struct Signals* sigs = &signals[get_core_num()];
    for (int i = 0; i < SIGNALS_SIZE; ++i) {
        int idx = __builtin_ffs(~sigs->mask[i]);
        if (idx > 0) {
            --idx;
            sigs->mask[i] |= 1u << idx;
            int sig = i * 32 + idx;
            set_handler(ls, sigs, sig, handler);
            uint32_t save = save_and_disable_interrupts();
            *psig = sig;
            restore_interrupts(save);
            return;
        }
    }
    luaL_error(ls, "no signals available");
}

void mlua_signal_unclaim(lua_State* ls, SigNum* psig) {
    SigNum sig = *psig;
    if (sig < 0) return;
    uint32_t save = save_and_disable_interrupts();
    *psig = -1;
    restore_interrupts(save);
    struct Signals* sigs = &signals[get_core_num()];
    lua_rawgetp(ls, LUA_REGISTRYINDEX, sigs);
    lua_pushinteger(ls, sig);
    lua_pushnil(ls);
    lua_settable(ls, -3);
    lua_pop(ls, 1);
    sigs->mask[sig / 32] &= ~(1u << (sig % 32));
}

void mlua_signal_set_handler(lua_State* ls, SigNum sig, int handler) {
    if (sig < 0) return;
    struct Signals* sigs = &signals[get_core_num()];
    if ((sigs->mask[sig / 32] & (1u << (sig % 32))) == 0) return;
    set_handler(ls, sigs, sig, handler);
}

bool mlua_signal_wrap_handler(lua_State* ls, lua_CFunction wrapper,
                              int handler) {
    if (lua_isnoneornil(ls, handler)) return false;
    if (wrapper == NULL) return true;
    lua_pushvalue(ls, handler);
    lua_pushcclosure(ls, wrapper, 1);
    lua_replace(ls, handler);
    return true;
}

void __time_critical_func(mlua_signal_set)(SigNum sig, bool pending) {
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
    absolute_time_t deadline = from_us_since_boot(mlua_check_int64(ls, 1));
    lua_rawgetp(ls, LUA_REGISTRYINDEX, sigs);
    uint32_t active[SIGNALS_SIZE];
    for (;;) {
        // Check for active signals and call their handlers.
        int found = SIGNALS_SIZE;
        sigs->wake = false;
        uint32_t save = save_and_disable_interrupts();
        for (int i = SIGNALS_SIZE - 1; i >= 0; --i) {
            uint32_t a = sigs->pending[i] & sigs->mask[i];
            active[i] = a;
            if (a != 0) found = i;
            sigs->pending[i] = 0;
        }
        restore_interrupts(save);
        if (found < SIGNALS_SIZE) {
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
        }

        // Return if at least one task is active or the deadline has elapsed.
        if (sigs->wake || is_nil_time(deadline) || time_reached(deadline)) {
            break;
        }

        // Wait for events, up to the deadline.
        if (!is_at_the_end_of_time(deadline)) {
            best_effort_wfe_or_timeout(deadline);
        } else {
            __wfe();
        }
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

int luaopen_mlua_signal(lua_State* ls) {
    mlua_require(ls, "mlua.int64", false);

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