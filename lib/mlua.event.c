#include "mlua/event.h"

#include <string.h>

#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/platform.h"
#include "pico/time.h"

#include "mlua/int64.h"
#include "mlua/util.h"

#define NUM_EVENTS 128
#define EVENTS_SIZE ((NUM_EVENTS + 31) / 32)

typedef struct Events {
    uint32_t pending[EVENTS_SIZE];
    uint32_t mask[EVENTS_SIZE];
} Events;

static Events events[NUM_CORES];

void mlua_event_claim(lua_State* ls, MLuaEvent* ev) {
    if (*ev < NUM_EVENTS) return;
    Events* evs = &events[get_core_num()];
    for (int i = 0; i < EVENTS_SIZE; ++i) {
        int idx = __builtin_ffs(~evs->mask[i]);
        if (idx > 0) {
            --idx;
            int e = i * 32 + idx;
            uint32_t save = save_and_disable_interrupts();
            *ev = e;
            restore_interrupts(save);
            evs->mask[i] |= 1u << idx;
            return;
        }
    }
    luaL_error(ls, "no events available");
}

void mlua_event_unclaim(lua_State* ls, MLuaEvent* ev) {
    MLuaEvent e = *ev;
    if (e >= NUM_EVENTS) return;
    uint32_t save = save_and_disable_interrupts();
    *ev = -1;
    restore_interrupts(save);
    Events* evs = &events[get_core_num()];
    lua_rawgetp(ls, LUA_REGISTRYINDEX, evs);
    lua_pushinteger(ls, e);
    lua_pushnil(ls);
    lua_rawset(ls, -3);
    lua_pop(ls, 1);
    evs->mask[e / 32] &= ~(1u << (e % 32));
}

void mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                           void (*handler)(void), int index,
                           lua_Integer priority) {
    int type = lua_type(ls, index);
    switch (type) {
    case LUA_TBOOLEAN:
        if (!lua_toboolean(ls, index)) {  // false => disable
            irq_set_enabled(irq, false);
            irq_remove_handler(irq, handler);
            mlua_event_unclaim(ls, ev);
            return;
        }
        break;
    case LUA_TNONE:
    case LUA_TNIL:
        break;
    default:
        priority = luaL_checkinteger(ls, index);
        break;
    }
    mlua_event_claim(ls, ev);
    if (priority < 0) {
        irq_set_exclusive_handler(irq, handler);
    } else {
        irq_add_shared_handler(irq, handler, priority);
    }
    irq_set_enabled(irq, true);
}

void __time_critical_func(mlua_event_set)(MLuaEvent ev, bool pending) {
    if (ev >= NUM_EVENTS) return;
    Events* evs = &events[get_core_num()];
    int index = ev / 32;
    uint32_t mask = 1u << (ev % 32);
    uint32_t save = save_and_disable_interrupts();
    if (pending) {
        evs->pending[index] |= mask;
    } else {
        evs->pending[index] &= ~mask;
    }
    restore_interrupts(save);
}

void mlua_event_watch(lua_State* ls, MLuaEvent ev) {
    if (ev >= NUM_EVENTS) {
        luaL_error(ls, "watching disabled event");
        return;
    }
    if (!lua_isyieldable(ls)) {
        luaL_error(ls, "watching event in unyieldable thread");
        return;
    }
    Events* evs = &events[get_core_num()];
    lua_rawgetp(ls, LUA_REGISTRYINDEX, evs);
    switch (lua_rawgeti(ls, -1, ev)) {
    case LUA_TNIL:  // No watchers
        lua_pop(ls, 1);
        lua_pushthread(ls);
        lua_rawseti(ls, -2, ev);
        lua_pop(ls, 1);
        return;
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (lua_rawequal(ls, -2, -1)) {  // Already registered
            lua_pop(ls, 3);
            return;
        }
        lua_createtable(ls, 0, 2);
        lua_rotate(ls, -3, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -4);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_rawseti(ls, -2, ev);
        lua_pop(ls, 1);
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pop(ls, 2);
        return;
    default:
        lua_pop(ls, 2);
        return;
    }
}

void mlua_event_unwatch(lua_State* ls, MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    Events* evs = &events[get_core_num()];
    lua_rawgetp(ls, LUA_REGISTRYINDEX, evs);
    switch (lua_rawgeti(ls, -1, ev)) {
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (!lua_rawequal(ls, -2, -1)) {  // Not the current thread
            lua_pop(ls, 3);
            return;
        }
        lua_pop(ls, 2);
        lua_pushnil(ls);
        lua_rawseti(ls, -2, ev);
        lua_pop(ls, 1);
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushnil(ls);
        lua_rawset(ls, -3);
        lua_pop(ls, 2);
        return;
    default:
        lua_pop(ls, 2);
        return;
    }
}

int mlua_event_suspend(lua_State* ls, lua_KFunction cont, int index) {
    if (index != 0) {
        lua_pushvalue(ls, index);
    } else {
        lua_pushboolean(ls, true);
    }
    lua_yieldk(ls, 1, 0, cont);
    return luaL_error(ls, "unable to yield");
}

static int mod_dispatch(lua_State* ls) {
    Events* evs = &events[get_core_num()];
    absolute_time_t deadline = from_us_since_boot(mlua_check_int64(ls, 2));
    lua_rawgetp(ls, LUA_REGISTRYINDEX, evs);
    for (;;) {
        // Check for active events and resume the watcher tasks.
        bool wake = false;
        for (int i = 0; i < EVENTS_SIZE; ++i) {
            uint32_t save = save_and_disable_interrupts();
            uint32_t active = evs->pending[i] & evs->mask[i];
            evs->pending[i] = 0;
            restore_interrupts(save);
            for (;;) {
                int idx = __builtin_ffs(active);
                if (idx == 0) break;
                --idx;
                active &= ~(1u << idx);
                switch (lua_rawgeti(ls, -1, i * 32 + idx)) {
                case LUA_TTHREAD:  // A single watcher
                    lua_pushvalue(ls, 1);
                    lua_rotate(ls, -2, 1);
                    lua_call(ls, 1, 1);
                    wake = wake || lua_toboolean(ls, -1);
                    lua_pop(ls, 1);
                    break;
                case LUA_TTABLE:  // Multiple watchers
                    lua_pushnil(ls);
                    while (lua_next(ls, -2)) {
                        lua_pop(ls, 1);
                        lua_pushvalue(ls, 1);
                        lua_pushvalue(ls, -2);
                        lua_call(ls, 1, 1);
                        wake = wake || lua_toboolean(ls, -1);
                        lua_pop(ls, 1);
                    }
                    lua_pop(ls, 1);
                    break;
                default:
                    lua_pop(ls, 1);
                    break;
                }
            }
        }

        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || is_nil_time(deadline) || time_reached(deadline)) {
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

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(dispatch),
#undef MLUA_SYM
    {NULL},
};

int luaopen_mlua_event(lua_State* ls) {
    mlua_require(ls, "mlua.int64", false);

    // Initialize internal state.
    Events* evs = &events[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    memset(evs, 0, sizeof(*evs));
    restore_interrupts(save);

    lua_newtable(ls);  // Watcher tasks table
    lua_rawsetp(ls, LUA_REGISTRYINDEX, evs);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
