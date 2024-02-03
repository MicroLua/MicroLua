// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/thread.h"

#include "pico/platform.h"

#include "mlua/platform.h"

// TODO: Move the event queue to the main thread extraspace

spin_lock_t* mlua_event_spinlock;
uint32_t mlua_event_lock_save;

static __attribute__((constructor)) void init(void) {
    mlua_event_spinlock = spin_lock_init(PICO_SPINLOCK_ID_OS1);
}

// A pending event queue. Events that are made pending are pushed to the tail,
// and event dispatching pops from the head. The queue is a linked list, where
// the state of each pending event contains a pointer to the next pending event,
// with the EVENT_PENDING bit set.
typedef struct EventQueue {
    MLuaEvent* head;
    MLuaEvent* tail;
} EventQueue;

#define EVENT_PENDING (1u << 0)

static inline bool is_pending(MLuaEvent const* ev) {
    return (ev->state & EVENT_PENDING) != 0;
}

static inline MLuaEvent* next_pending(MLuaEvent const* ev) {
    return (MLuaEvent*)(ev->state & ~EVENT_PENDING);
}

static uint8_t queue;  // Just a marker for a registry entry

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
    mlua_event_lock();
    if (ev->state != 0) {
        mlua_event_unlock();
        return false;
    }
    ev->state = (uintptr_t)q;
    mlua_event_unlock();
    return true;
}

void mlua_event_disable(lua_State* ls, MLuaEvent* ev) {
    EventQueue* q = get_queue(ls);
    mlua_event_lock();
    if (ev->state == 0) {
        mlua_event_unlock();
        return;
    }
    if (is_pending(ev)) remove_pending_nolock(q, ev);
    ev->state = 0;
    mlua_event_unlock();
    mlua_event_remove_watcher(ls, ev);
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

void mlua_event_dispatch(lua_State* ls, uint64_t deadline, MLuaResume resume) {
    uint64_t ticks_min, ticks_max;
    mlua_ticks_range(&ticks_min, &ticks_max);
    bool wake = deadline == ticks_min;
    EventQueue* q = get_queue(ls);
    for (;;) {
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
            if (mlua_event_resume_watcher(ls, ev, resume)) wake = true;
        }

        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || mlua_ticks_reached(deadline)) return;
        wake = false;

        // Wait for events, up to the deadline.
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
