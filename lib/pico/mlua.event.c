// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/event.h"

#include "mlua/int64.h"
#include "mlua/module.h"
#include "mlua/platform.h"
#include "mlua/util.h"

// TODO: Make some short functions inline
// TODO: Don't store the handler thread in the registry; it should already be
//       available as a (single) watcher; start watching in the parent
// TODO: Combine enabling an event and watching an event => always a single
//       watcher for each event. This might be tricky, as enabling has a locking
//       function, but it might be doable
// TODO: Make yield_enabled per-thread
// TODO: Add "performance" counters: dispatch cycles, sleeps

void mlua_event_require(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);
}

spin_lock_t* mlua_event_spinlock;
static uint8_t queue;  // Just a marker for a registry entry
static uint8_t yield_disabled;  // Just a marker for a registry entry

// A pending event queue. Events that are made pending are pushed to the tail,
// and event dispatching pops from the head. The queue is a linked list, where
// the state of each pending event contains a pointer to the next pending event,
// with the EVENT_PENDING bit set.
typedef struct EventQueue {
    MLuaEvent* head;
    MLuaEvent* tail;
} EventQueue;

#define EVENT_PENDING (1 << 0)

static inline bool is_pending(MLuaEvent const* ev) {
    return (ev->state & EVENT_PENDING) != 0;
}

static inline MLuaEvent* next_pending(MLuaEvent const* ev) {
    return (MLuaEvent*)(ev->state & ~EVENT_PENDING);
}

static inline void const* watchers_tag(MLuaEvent const* ev) { return ev; }

static inline void const* handler_tag(MLuaEvent const* ev) {
    return (void const*)ev + 1;
}

EventQueue* get_queue(lua_State* ls) {
    EventQueue* q;
    if (lua_rawgetp(ls, LUA_REGISTRYINDEX, &queue) == LUA_TNIL) {
        q = lua_newuserdatauv(ls, sizeof(EventQueue), 0);
        q->head = NULL;
        lua_rawsetp(ls, LUA_REGISTRYINDEX, &queue);
    } else {
        q = lua_touserdata(ls, -1);
    }
    lua_pop(ls, 1);
    return q;
}

static void remove_pending_nolock(EventQueue* q, MLuaEvent const* ev) {
    if (q->head == ev) {
        q->head = next_pending(ev);
    } else {
        for (MLuaEvent* cur = q->head;;) {
            MLuaEvent* next = next_pending(cur);
            if (next == NULL) break;
            if (next == ev) {
                cur->state = ev->state;
                break;
            }
            cur = next;
        }
    }
}

bool mlua_event_enable(lua_State* ls, MLuaEvent* ev) {
    EventQueue* q = get_queue(ls);
    uint32_t save = mlua_event_lock();
    if (ev->state != 0) {
        mlua_event_unlock(save);
        return false;
    }
    ev->state = (uintptr_t)q;
    mlua_event_unlock(save);
    return true;
}

void mlua_event_disable(lua_State* ls, MLuaEvent* ev) {
    EventQueue* q = get_queue(ls);
    uint32_t save = mlua_event_lock();
    if (ev->state == 0) {
        mlua_event_unlock(save);
        return;
    }
    if (is_pending(ev)) remove_pending_nolock(q, ev);
    ev->state = 0;
    mlua_event_unlock(save);
    // TODO: Resume watchers so that they can exit
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
}

bool mlua_event_enabled_nolock(MLuaEvent const* ev) {
    return ev->state != 0;
}

bool mlua_event_enabled(MLuaEvent const* ev) {
    uint32_t save = mlua_event_lock();
    bool en = ev->state != 0;
    mlua_event_unlock(save);
    return en;
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

bool mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                           irq_handler_t handler, int index,
                           lua_Integer priority) {
    if (!mlua_event_enable_irq_arg(ls, index, &priority)) {  // Disable IRQ
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, handler);
        mlua_event_disable(ls, ev);
        return true;
    }
    if (!mlua_event_enable(ls, ev)) return false;
    mlua_event_set_irq_handler(irq, handler, priority);
    irq_set_enabled(irq, true);
    return true;
}

void __time_critical_func(mlua_event_set_nolock)(MLuaEvent* ev) {
    if (ev->state == 0 || is_pending(ev)) return;
    EventQueue* q = (EventQueue*)ev->state;
    ev->state = (uintptr_t)NULL | EVENT_PENDING;
    if (q->head == NULL) {
        q->head = q->tail = ev;
    } else {
        q->tail->state = (uintptr_t)ev | EVENT_PENDING;
        q->tail = ev;
    }
    __sev();
}

void __time_critical_func(mlua_event_set)(MLuaEvent* ev) {
    uint32_t save = mlua_event_lock();
    mlua_event_set_nolock(ev);
    mlua_event_unlock(save);
}

void mlua_event_watch(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) {
        luaL_error(ls, "watching disabled event");
        return;
    }
    if (!lua_isyieldable(ls)) {
        luaL_error(ls, "watching event in unyieldable thread");
        return;
    }
    switch (lua_rawgetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev))) {
    case LUA_TNIL:  // No watchers
        lua_pop(ls, 1);
        lua_pushthread(ls);
        lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
        return;
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (lua_rawequal(ls, -2, -1)) {  // Already registered
            lua_pop(ls, 2);
            return;
        }
        lua_createtable(ls, 0, 2);
        lua_rotate(ls, -2, 1);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -2);
        lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushboolean(ls, true);
        lua_rawset(ls, -3);
        lua_pop(ls, 1);
        return;
    default:
        lua_pop(ls, 1);
        return;
    }
}

void mlua_event_unwatch(lua_State* ls, MLuaEvent const* ev) {
    if (!mlua_event_enabled(ev)) return;
    switch (lua_rawgetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev))) {
    case LUA_TTHREAD:  // A single watcher
        lua_pushthread(ls);
        if (!lua_rawequal(ls, -2, -1)) {  // Not the current thread
            lua_pop(ls, 2);
            return;
        }
        lua_pop(ls, 2);
        lua_pushnil(ls);
        lua_rawsetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev));
        return;
    case LUA_TTABLE:  // Multiple watchers
        lua_pushthread(ls);
        lua_pushnil(ls);
        lua_rawset(ls, -3);
        lua_pop(ls, 1);
        return;
    default:
        lua_pop(ls, 1);
        return;
    }
}

int mlua_event_yield(lua_State* ls, int nresults, lua_KFunction cont,
                     lua_KContext ctx) {
    lua_yieldk(ls, nresults, ctx, cont);
    return cont(ls, LUA_OK, ctx);
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

bool mlua_yield_enabled(lua_State* ls) {
    bool dis = lua_rawgetp(ls, LUA_REGISTRYINDEX, &yield_disabled) != LUA_TNIL;
    lua_pop(ls, 1);
    return !dis;
}

void mlua_set_yield_enabled(lua_State* ls, bool en) {
    if (en) {
        lua_pushnil(ls);
    } else {
        lua_pushboolean(ls, true);
    }
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &yield_disabled);
}

bool mlua_event_can_wait(lua_State* ls, MLuaEvent const* ev) {
    return mlua_yield_enabled(ls) && mlua_event_enabled(ev);
}

static int mlua_event_loop_1(lua_State* ls, MLuaEvent const* ev,
                             MLuaEventLoopFn loop, int index);
static int mlua_event_loop_2(lua_State* ls, int status, lua_KContext ctx);

int mlua_event_loop(lua_State* ls, MLuaEvent const* ev, MLuaEventLoopFn loop,
                    int index) {
    if (!mlua_event_enabled(ev)) {
        return luaL_error(ls, "wait for disabled event");
    }
    int res = loop(ls, false);
    if (res >= 0) return res;
    if (index != 0) {
        if (lua_isnoneornil(ls, index)) {
            index = 0;
        } else {
            mlua_check_int64(ls, index);
        }
    }
    mlua_event_watch(ls, ev);
    return mlua_event_loop_1(ls, ev, loop, index);
}

static int mlua_event_loop_1(lua_State* ls, MLuaEvent const* ev,
                             MLuaEventLoopFn loop, int index) {
    lua_pushlightuserdata(ls, loop);
    lua_pushinteger(ls, index);
    return mlua_event_suspend(ls, &mlua_event_loop_2, (lua_KContext)ev, index);
}

static int mlua_event_loop_2(lua_State* ls, int status, lua_KContext ctx) {
    MLuaEventLoopFn loop = (MLuaEventLoopFn)lua_touserdata(ls, -2);
    int index = lua_tointeger(ls, -1);
    lua_pop(ls, 2);  // Restore the stack for loop
    int res = loop(ls, false);
    if (res < 0) {
        if (index == 0 ||
                !mlua_platform_ticks_reached(mlua_to_int64(ls, index))) {
            return mlua_event_loop_1(ls, (MLuaEvent*)ctx, loop, index);
        }
        res = loop(ls, true);
    }
    mlua_event_unwatch(ls, (MLuaEvent const*)ctx);
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
    mlua_event_watch(ls, event);
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
    MLuaEvent* ev = lua_touserdata(ls, lua_upvalueindex(2));
    mlua_event_unwatch(ls, ev);
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, handler_tag(ev));

    // Call the "handler done" callback.
    if (!lua_isnil(ls, lua_upvalueindex(1))) {
        lua_pushvalue(ls, lua_upvalueindex(1));
        lua_callk(ls, 0, 0, 0, &mlua_cont_return_ctx);
    }
    return 0;
}

int mlua_event_handle(lua_State* ls, MLuaEvent* ev, lua_KFunction cont,
                      lua_KContext ctx) {
    lua_pushlightuserdata(ls, ev);
    lua_pushcclosure(ls, &handler_thread, 3);
    mlua_thread_start(ls);
    lua_pushvalue(ls, -1);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, handler_tag(ev));
    return mlua_event_yield(ls, 0, cont, ctx);
}

void mlua_event_stop_handler(lua_State* ls, MLuaEvent const* ev) {
    if (lua_rawgetp(ls, LUA_REGISTRYINDEX, handler_tag(ev)) == LUA_TTHREAD) {
        mlua_thread_kill(ls);
    } else {
        lua_pop(ls, 1);
    }
}

int mlua_event_push_handler_thread(lua_State* ls, MLuaEvent const* ev) {
    return lua_rawgetp(ls, LUA_REGISTRYINDEX, handler_tag(ev));
}

static int mod_dispatch(lua_State* ls) {
    uint64_t deadline = mlua_check_int64(ls, 1);
    uint64_t ticks_min, ticks_max;
    mlua_platform_ticks_range(&ticks_min, &ticks_max);
    bool wake = deadline == ticks_min || mlua_platform_ticks_reached(deadline);

    // Get Thread.resume.
    lua_pushthread(ls);
    luaL_getmetafield(ls, -1, "resume");
    lua_replace(ls, 1);
    lua_settop(ls, 1);

    EventQueue* q = get_queue(ls);
    for (;;) {
        // Check for pending events and resume the corresponding watcher
        // threads.
        for (;;) {
            uint32_t save = mlua_event_lock();
            MLuaEvent* ev = q->head;
            if (ev != NULL) {
                q->head = next_pending(ev);
                ev->state = (uintptr_t)q;
            }
            mlua_event_unlock(save);
            if (ev == NULL) break;
            switch (lua_rawgetp(ls, LUA_REGISTRYINDEX, watchers_tag(ev))) {
            case LUA_TTHREAD:  // A single watcher
                lua_pushvalue(ls, 1);  // Thread.resume
                lua_rotate(ls, -2, 1);
                lua_call(ls, 1, 1);
                wake = wake || lua_toboolean(ls, -1);
                lua_pop(ls, 1);
                break;
            case LUA_TTABLE:  // Multiple watchers
                lua_pushnil(ls);
                while (lua_next(ls, -2)) {
                    lua_pop(ls, 1);
                    lua_pushvalue(ls, 1);  // Thread.resume
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

        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || mlua_platform_ticks_reached(deadline)) break;
        wake = false;

        // Wait for events, up to the deadline.
        mlua_platform_wait(deadline);
    }
    return 0;
}

int mod_set_thread_metatable(lua_State* ls) {
    lua_pushthread(ls);
    lua_pushvalue(ls, 1);
    lua_setmetatable(ls, -2);
    return 0;
}

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_F(dispatch, mod_),
    MLUA_SYM_F(set_thread_metatable, mod_),
};

static __attribute__((constructor)) void init(void) {
    mlua_event_spinlock = spin_lock_instance(next_striped_spin_lock_num());
}

MLUA_OPEN_MODULE(mlua.event) {
    mlua_require(ls, "mlua.int64", false);
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
