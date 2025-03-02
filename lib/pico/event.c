// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/thread.h"

#include <assert.h>

#include "pico.h"

#include "lstate.h"
#include "mlua/module.h"
#include "mlua/platform.h"

spin_lock_t* mlua_event_spinlock;
uint32_t mlua_event_lock_save;

static __attribute__((constructor)) void init(void) {
    mlua_event_spinlock = spin_lock_init(PICO_SPINLOCK_ID_OS1);
}

// A pending event queue. Events that are made pending are pushed to the tail,
// and event dispatching pops from the head. The queue is a linked list, where
// the state of each pending event contains a pointer to the next pending event,
// with the lower bits set to EVENT_PENDING.
typedef struct EventQueue {
    MLuaEvent* head;
    MLuaEvent* tail;
} EventQueue;

static_assert(sizeof(EventQueue) <= LUA_EXTRASPACE, "LUA_EXTRASPACE too small");

typedef enum EventState {
    EVENT_IDLE = 0,
    EVENT_PENDING = 1,
    EVENT_ABANDONED = 2,
    EVENT_MASK = 3,
} EventState;

static inline EventState event_state(MLuaEvent const* ev) {
    return ev->state & EVENT_MASK;
}

static inline MLuaEvent* next_pending(MLuaEvent const* ev) {
    return (MLuaEvent*)(ev->state & ~EVENT_MASK);
}

static inline EventQueue* get_queue(lua_State* ls) {
#ifdef mainthread  // Defined in lstate.h in Lua >=5.5
    lua_State* main = mainthread(G(ls));
#else
    lua_State* main = G(ls)->mainthread;
#endif
    return (EventQueue*)lua_getextraspace(main);
}

static void remove_pending_nolock(EventQueue* q, MLuaEvent const* ev) {
    if (q->head == ev) {
        q->head = next_pending(ev);
        return;
    }
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

bool mlua_event_enable(lua_State* ls, MLuaEvent* ev) {
    EventQueue* q = get_queue(ls);
    mlua_event_lock();
    if (ev->state != 0) {
        mlua_event_unlock();
        return false;
    }
    ev->state = (uintptr_t)q;
    mlua_event_unlock();
    return true;
}

void disable_event(lua_State* ls, MLuaEvent* ev, uintptr_t state) {
    mlua_event_lock();
    if (ev->state == 0) {
        mlua_event_unlock();
        return;
    }
    if (event_state(ev) == EVENT_PENDING) {
        remove_pending_nolock(get_queue(ls), ev);
    }
    ev->state = state;
    mlua_event_unlock();
    mlua_event_remove_watcher(ls, ev);
}

void mlua_event_abandon(lua_State* ls, MLuaEvent* ev) {
    disable_event(ls, ev, EVENT_ABANDONED);
}

void mlua_event_disable(lua_State* ls, MLuaEvent* ev) {
    disable_event(ls, ev, 0);
}

void __time_critical_func(mlua_event_set_nolock)(MLuaEvent* ev) {
    if (ev->state == 0 || event_state(ev) != EVENT_IDLE) return;
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

bool mlua_event_disable_abandoned(MLuaEvent* ev) {
    mlua_event_lock();
    bool res = ev->state == EVENT_ABANDONED;
    if (res) ev->state = 0;
    mlua_event_unlock();
    return res;
}

void mlua_event_dispatch(lua_State* ls, uint64_t deadline) {
    bool wake = deadline == MLUA_TICKS_MIN;
    EventQueue* q = get_queue(ls);
#if MLUA_THREAD_STATS
    MLuaGlobal* g = mlua_global(ls);
#endif
    for (;;) {
#if MLUA_THREAD_STATS
        ++g->thread_dispatches;
#endif

        // Check for pending events and resume their watchers.
        for (;;) {
            mlua_event_lock();
            MLuaEvent* ev = q->head;
            if (ev != NULL) {
                q->head = next_pending(ev);
                ev->state = (uintptr_t)q;
            }
            mlua_event_unlock();
            if (ev == NULL) break;
            if (mlua_event_resume_watcher(ls, ev)) wake = true;
        }

        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || mlua_ticks64_reached(deadline)) return;
        wake = false;

        // Wait for events, up to the deadline.
#if MLUA_THREAD_STATS
        ++g->thread_waits;
#endif
        mlua_wait(deadline);
    }
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
