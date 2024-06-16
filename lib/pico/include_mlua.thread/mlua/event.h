// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_MLUA_THREAD_EVENT_H
#define _MLUA_LIB_PICO_MLUA_THREAD_EVENT_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/types.h"

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Lock event handling. This disables interrupts.
__attribute__((__always_inline__))
static inline void mlua_event_lock(void) {
    extern spin_lock_t* mlua_event_spinlock;
    extern uint32_t mlua_event_lock_save;
    mlua_event_lock_save = spin_lock_blocking(mlua_event_spinlock);
}

// Unlock event handling.
__attribute__((__always_inline__))
static inline void mlua_event_unlock(void) {
    extern spin_lock_t* mlua_event_spinlock;
    extern uint32_t mlua_event_lock_save;
    spin_unlock(mlua_event_spinlock, mlua_event_lock_save);
}

// An event. The state is a tagged union of two pointers:
//  - If the state is zero, the event is disabled.
//  - If the lower bits are EVENT_IDLE, the event is enabled and the state
//    contains a pointer to the pending event queue.
//  - If the lower bits are EVENT_PENDING, the event is pending and the state
//    contains a pointer to the next pending event in the queue.
//  - If the lower bits are EVENT_ABANDONED, the event was abandoned, and can be
//    disabled with mlua_event_disable_abandoned().
typedef struct MLuaEvent {
    uintptr_t state;
} MLuaEvent;

// Initialize an event.
inline void mlua_event_init(MLuaEvent* ev) { ev->state = 0; }

// Enable an event. Returns false iff the event was already enabled.
bool mlua_event_enable(lua_State* ls, MLuaEvent* ev);

// Abandon an event. This keeps the event enabled, but allows disabling it from
// a non-Lua context with mlua_event_disable_abandoned().
void mlua_event_abandon(lua_State* ls, MLuaEvent* ev);

// Disable an event.
void mlua_event_disable(lua_State* ls, MLuaEvent* ev);

// Return true iff the event is enabled. Must be in a locked section.
static inline bool mlua_event_enabled_nolock(MLuaEvent const* ev) {
    return ev->state != 0;
}

// Return true iff the event is enabled.
static inline bool mlua_event_enabled(MLuaEvent const* ev) {
    mlua_event_lock();
    bool en = ev->state != 0;
    mlua_event_unlock();
    return en;
}

// Set an event pending. Must be in a locked section.
void mlua_event_set_nolock(MLuaEvent* ev);

// Set an event pending.
static inline void mlua_event_set(MLuaEvent* ev) {
    mlua_event_lock();
    mlua_event_set_nolock(ev);
    mlua_event_unlock();
}

// Disable an event, and return true, iff the event has been abandoned.
bool mlua_event_disable_abandoned(MLuaEvent* ev);

// Dispatch pending events.
void mlua_event_dispatch(lua_State* ls, uint64_t deadline);

// Parse an IRQ priority argument, which must be an integer or nil.
lua_Integer mlua_event_parse_irq_priority(lua_State* ls, int arg,
                                          lua_Integer def);

// Parse the enable_irq argument.
bool mlua_event_enable_irq_arg(lua_State* ls, int arg, lua_Integer* priority);

// Set an IRQ handler.
void mlua_event_set_irq_handler(uint irq, irq_handler_t handler,
                                lua_Integer priority);

// Enable or disable IRQ handling. The argument at the given index determines
// what is done:
//  - true, nil, none, integer: Enable the event, set the IRQ handler and enable
//    the IRQ. If the argument is a non-negative integer, add a shared handler
//    with the given priority. Otherwise, set an exclusive handler.
//  - false: Disable the IRQ, remove the IRQ handler and disable the event.
// False iff the event was already enabled.
bool mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                           irq_handler_t handler, int index,
                           lua_Integer priority);

#ifdef __cplusplus
}
#endif

#endif
