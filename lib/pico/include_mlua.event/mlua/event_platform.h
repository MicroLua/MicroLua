// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_EVENT_PLATFORM_H
#define _MLUA_LIB_PICO_EVENT_PLATFORM_H

#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/types.h"

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

// Wake the interpreter that is watching the event.
__attribute__((__always_inline__))
static inline void mlua_event_wake(void) { __sev(); }

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
