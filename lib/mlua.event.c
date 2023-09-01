#include "mlua/event.h"

#include "pico/platform.h"
#include "pico/time.h"

#include "mlua/int64.h"
#include "mlua/util.h"

// TODO: Add "performance" counters: dispatch cycles, sleeps

void mlua_event_require(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);
}

spin_lock_t* mlua_event_spinlock;

#define NUM_EVENTS 128
#define EVENTS_SIZE ((NUM_EVENTS + 31) / 32)

typedef struct EventState {
    uint32_t pending[EVENTS_SIZE];
    uint32_t mask[NUM_CORES][EVENTS_SIZE];
} EventState;

static EventState event_state;

char const* const mlua_event_err_already_claimed
    = "event already claimed";

char const* mlua_event_claim_core(MLuaEvent* ev, uint core) {
    uint32_t save = mlua_event_lock();
    MLuaEvent e = *ev;
    if (e < NUM_EVENTS) {
        mlua_event_unlock(save);
        return mlua_event_err_already_claimed;
    }
    for (uint block = 0; block < EVENTS_SIZE; ++block) {
        uint32_t mask = 0;
        for (uint c = 0; c < NUM_CORES; ++c) mask |= event_state.mask[c][block];
        int idx = __builtin_ffs(~mask);
        if (idx > 0) {
            --idx;
            *ev = block * 32 + idx;
            uint32_t m = 1u << idx;
            event_state.pending[block] &= ~m;
            event_state.mask[core][block] |= m;
            mlua_event_unlock(save);
            return NULL;
        }
    }
    mlua_event_unlock(save);
    return "no events available";
}

void mlua_event_unclaim(lua_State* ls, MLuaEvent* ev) {
    uint32_t save = mlua_event_lock();
    MLuaEvent e = *ev;
    if (e >= NUM_EVENTS) {
        mlua_event_unlock(save);
        return;
    }
    uint32_t* pmask = &event_state.mask[get_core_num()][e / 32];
    uint32_t mask = *pmask & ~(1u << (e % 32));
    if (mask == *pmask) {
        mlua_event_unlock(save);
        return;
    }
    *pmask = mask;
    *ev = MLUA_EVENT_UNSET;
    mlua_event_unlock(save);
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
    lua_pushnil(ls);
    lua_rawseti(ls, -2, e);
    lua_pop(ls, 1);
}

lua_Integer mlua_event_parse_irq_priority(lua_State* ls, int arg,
                                          lua_Integer def) {
    switch (lua_type(ls, arg)) {
    case LUA_TNONE:
    case LUA_TNIL:
        return def;
    case LUA_TNUMBER:
        lua_Integer priority = luaL_checkinteger(ls, arg);
        luaL_argcheck(
            ls, priority <= PICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY,
            arg, "invalid priority");
        return priority;
    default:
        return luaL_typeerror(ls, arg, "integer or nil");
    }
}

bool mlua_event_enable_irq_arg(lua_State* ls, int arg,
                               lua_Integer* priority) {
    if (lua_isboolean(ls, arg)) return lua_toboolean(ls, arg);
    *priority = mlua_event_parse_irq_priority(ls, arg, *priority);
    return true;
}

void mlua_event_set_irq_handler(uint irq, void (*handler)(void),
                                lua_Integer priority) {
    if (priority < 0) {
        irq_set_exclusive_handler(irq, handler);
    } else {
        irq_add_shared_handler(irq, handler, priority);
    }
}

char const* mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                                  void (*handler)(void), int index,
                                  lua_Integer priority) {
    if (!mlua_event_enable_irq_arg(ls, index, &priority)) {  // Disable IRQ
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, handler);
        mlua_event_unclaim(ls, ev);
        return NULL;
    }
    char const* err = mlua_event_claim(ev);
    if (err != NULL) return err;
    mlua_event_set_irq_handler(irq, handler, priority);
    irq_set_enabled(irq, true);
    return NULL;
}

void __time_critical_func(mlua_event_set)(MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    uint32_t save = mlua_event_lock();
    event_state.pending[ev / 32] |= 1u << (ev % 32);
    mlua_event_unlock(save);
    __sev();
}

void __time_critical_func(mlua_event_set_nolock)(MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    event_state.pending[ev / 32] |= 1u << (ev % 32);
    __sev();
}

void mlua_event_clear(MLuaEvent ev) {
    if (ev >= NUM_EVENTS) return;
    uint32_t save = mlua_event_lock();
    event_state.pending[ev / 32] &= ~(1u << (ev % 32));
    mlua_event_unlock(save);
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
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
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
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
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

int mlua_event_yield(lua_State* ls, int nresults, lua_KFunction cont,
                     lua_KContext ctx) {
    lua_yieldk(ls, nresults, ctx, cont);
    return luaL_error(ls, "unable to yield");
}

int mlua_event_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                       int index) {
    if (index != 0) {
        lua_pushvalue(ls, index);
    } else {
        lua_pushboolean(ls, true);
    }
    return mlua_event_yield(ls, 1, cont, ctx);
}

bool mlua_event_can_wait(MLuaEvent* event) {
    if (!mlua_yield_enabled()) return false;
    uint32_t save = mlua_event_lock();
    MLuaEvent e = *event;
    bool ok = e < NUM_EVENTS && (event_state.mask[get_core_num()][e / 32]
                                 & (1u << (e % 32))) != 0;
    mlua_event_unlock(save);
    return ok;
}

static int mlua_event_wait_1(lua_State* ls, MLuaEvent event,
                             MLuaEventGetter try_get, int index);
static int mlua_event_wait_2(lua_State* ls, int status, lua_KContext ctx);

int mlua_event_wait(lua_State* ls, MLuaEvent event, MLuaEventGetter try_get,
                    int index) {
    if (event >= NUM_EVENTS) return luaL_error(ls, "wait for unclaimed event");
    int res = try_get(ls, false);
    if (res >= 0) return res;
    mlua_event_watch(ls, event);
    return mlua_event_wait_1(ls, event, try_get, index);
}

static int mlua_event_wait_1(lua_State* ls, MLuaEvent event,
                             MLuaEventGetter try_get, int index) {
    lua_pushlightuserdata(ls, try_get);
    lua_pushinteger(ls, index);
    return mlua_event_suspend(ls, &mlua_event_wait_2, event, index);
}

static int mlua_event_wait_2(lua_State* ls, int status, lua_KContext ctx) {
    MLuaEventGetter try_get = (MLuaEventGetter)lua_touserdata(ls, -2);
    int index = lua_tointeger(ls, -1);
    lua_pop(ls, 2);  // Restore the stack for try_get
    int res = try_get(ls, false);
    if (res < 0) {
        if (index == 0 || !time_reached(mlua_check_int64(ls, index))) {
            return mlua_event_wait_1(ls, (MLuaEvent)ctx, try_get, index);
        }
        res = try_get(ls, true);
    }
    mlua_event_unwatch(ls, (MLuaEvent)ctx);
    return res;
}

static int handler_thread_1(lua_State* ls, int status, lua_KContext ctx);
static int handler_thread_2(lua_State* ls, int status, lua_KContext ctx);
static int handler_thread_done(lua_State* ls);

static int handler_thread(lua_State* ls) {
    // Set up a deferred to clean up on exit.
    lua_pushvalue(ls, lua_upvalueindex(2));  // done
    lua_pushvalue(ls, lua_upvalueindex(3));  // event
    lua_pushcclosure(ls, &handler_thread_done, 2);
    lua_toclose(ls, -1);

    // Watch the event.
    MLuaEvent* event = lua_touserdata(ls, lua_upvalueindex(3));
    mlua_event_watch(ls, *event);
    return handler_thread_1(ls, LUA_OK, 0);
}

static int handler_thread_1(lua_State* ls, int status, lua_KContext ctx) {
    // Call the event handler.
    lua_pushvalue(ls, lua_upvalueindex(1));  // handler
    lua_callk(ls, 0, 0, 0, &handler_thread_2);
    return handler_thread_2(ls, LUA_OK, 0);
}

static int handler_thread_2(lua_State* ls, int status, lua_KContext ctx) {
    // Suspend until the event gets pending.
    lua_pushboolean(ls, true);
    return mlua_event_yield(ls, 1, &handler_thread_1, 0);
}

static int handler_thread_done(lua_State* ls) {
    // Stop watching the event.
    MLuaEvent* event = lua_touserdata(ls, lua_upvalueindex(2));
    mlua_event_unwatch(ls, *event);
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, event);

    // Call the "handler done" callback.
    if (!lua_isnil(ls, lua_upvalueindex(1))) {
        lua_pushvalue(ls, lua_upvalueindex(1));
        lua_callk(ls, 0, 0, 0, &mlua_cont_return_ctx);
    }
    return 0;
}

void mlua_event_handle(lua_State* ls, MLuaEvent* event) {
    lua_pushlightuserdata(ls, event);
    lua_pushcclosure(ls, &handler_thread, 3);
    mlua_thread_start(ls);
    lua_pushvalue(ls, -1);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, event);
}

void mlua_event_stop_handler(lua_State* ls, MLuaEvent* event) {
    if (lua_rawgetp(ls, LUA_REGISTRYINDEX, event) == LUA_TTHREAD) {
        mlua_thread_kill(ls);
    } else {
        lua_pop(ls, 1);
    }
}

static int mod_dispatch(lua_State* ls) {
    absolute_time_t deadline = from_us_since_boot(mlua_check_int64(ls, 1));

    // Get Thread.resume.
    lua_pushthread(ls);
    luaL_getmetafield(ls, -1, "resume");
    lua_replace(ls, 1);
    lua_settop(ls, 1);

    lua_rawgetp(ls, LUA_REGISTRYINDEX, &event_state);
    uint32_t* masks = event_state.mask[get_core_num()];
    for (;;) {
        // Check for pending events and resume the corresponding watcher
        // threads.
        bool wake = false;
        for (uint block = 0; block < EVENTS_SIZE; ++block) {
            uint32_t* pending = &event_state.pending[block];
            uint32_t save = mlua_event_lock();
            uint32_t active = *pending;
            uint32_t mask = masks[block];
            *pending = active & ~mask;
            mlua_event_unlock(save);
            active &= mask;
            for (;;) {
                int idx = __builtin_ffs(active);
                if (idx == 0) break;
                --idx;
                active &= ~(1u << idx);
                switch (lua_rawgeti(ls, 2, block * 32 + idx)) {
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
        if (wake || is_nil_time(deadline) || time_reached(deadline)) break;

        // Wait for events, up to the deadline.
        if (!is_at_the_end_of_time(deadline)) {
            best_effort_wfe_or_timeout(deadline);
        } else {
            __wfe();
        }
    }
    return 0;
}

int mod_set_thread_metatable(lua_State* ls) {
    lua_pushthread(ls);
    lua_pushvalue(ls, 1);
    lua_setmetatable(ls, -2);
    return 0;
}

static MLuaSym const module_syms[] = {
    MLUA_SYM_F(dispatch, mod_),
    MLUA_SYM_F(set_thread_metatable, mod_),
};

static __attribute__((constructor)) void init(void) {
    mlua_event_spinlock = spin_lock_instance(next_striped_spin_lock_num());
}

int luaopen_mlua_event(lua_State* ls) {
    mlua_require(ls, "mlua.int64", false);

    // Create the watcher thread table.
    lua_createtable(ls, NUM_EVENTS, 0);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &event_state);

    // Create the module.
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
