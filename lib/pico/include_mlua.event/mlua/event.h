// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_EVENT_H
#define _MLUA_LIB_PICO_EVENT_H

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

// Require the mlua.event module.
void mlua_event_require(lua_State* ls);

// An event identifier.
typedef uint8_t MLuaEvent;

// A marker for "no event assigned".
#define MLUA_EVENT_UNSET ((MLuaEvent)-1)

// The error returned by mlua_event_claim*() if the event was already claimed.
extern char const* const mlua_event_err_already_claimed;

// Claim an event for the given core. Returns NULL on success, or a message
// describing the error.
char const* mlua_event_claim_core(lua_State* ls, MLuaEvent* ev, uint core);

// Claim an event for the calling core. Returns NULL on success, or a message
// describing the error.
static inline char const* mlua_event_claim(lua_State* ls, MLuaEvent* ev) {
    return mlua_event_claim_core(ls, ev, get_core_num());
}

// Free an event.
void mlua_event_unclaim(lua_State* ls, MLuaEvent* ev);

// Lock event handling. This disables interrupts.
__force_inline static uint32_t mlua_event_lock(void) {
    extern spin_lock_t* mlua_event_spinlock;
    return spin_lock_blocking(mlua_event_spinlock);
}

// Unlock event handling.
__force_inline static void mlua_event_unlock(uint32_t save) {
    extern spin_lock_t* mlua_event_spinlock;
    spin_unlock(mlua_event_spinlock, save);
}

// Dereference an MLuaEvent* under the event lock.
__force_inline static MLuaEvent mlua_event_deref(MLuaEvent* ev) {
    uint32_t save = mlua_event_lock();
    MLuaEvent e = *ev;
    mlua_event_unlock(save);
    return e;
}

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
//  - true, nil, none, integer: Claim an event, set the IRQ handler and enable
//    the IRQ. If the argument is a non-negative integer, add a shared handler
//    with the given priority. Otherwise, set an exclusive handler.
//  - false: Disable the IRQ, remove the IRQ handler and unclaim the event.
// Returns NULL on success, or a message describing the error.
char const* mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                                  irq_handler_t handler, int index,
                                  lua_Integer priority);

// Set an event pending.
void mlua_event_set(MLuaEvent* ev);

// Set an event pending. Must be in a locked section.
void mlua_event_set_nolock(MLuaEvent* ev);

// Clear the pending state of an event.
void mlua_event_clear(MLuaEvent* ev);

// Register the current thread to be notified when an event triggers.
void mlua_event_watch(lua_State* ls, MLuaEvent* ev);

// Unregister the current thread from notifications for an event.
void mlua_event_unwatch(lua_State* ls, MLuaEvent* ev);

// Yield from the running thread.
int mlua_event_yield(lua_State* ls, int nresults, lua_KFunction cont,
                     lua_KContext ctx);

// Suspend the running thread. If the given index is non-zero, yield the value
// at that index, which must be a deadline in microseconds. Otherwise, yield
// false to suspend indefinitely.
int mlua_event_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                       int index);

typedef int (*MLuaEventLoopFn)(lua_State*, bool);

#if LIB_MLUA_MOD_MLUA_EVENT
// Return true iff yielding is enabled.
bool mlua_yield_enabled(lua_State* ls);
void mlua_set_yield_enabled(lua_State* ls, bool en);
// TODO: Allow force-enabling yielding => eliminate blocking code
// TODO: Make yield status per-thread
#else
__attribute__((__always_inline__))
static inline bool mlua_yield_enabled(lua_State* ls) { return false; }
__attribute__((__always_inline__))
static inline void mlua_set_yield_enabled(lua_State* ls, bool en) {}
#endif

// Return true iff waiting for the given even is possible, i.e. yielding is
// enabled and the event was claimed.
bool mlua_event_can_wait(lua_State* ls, MLuaEvent* event);

// Run an event loop. The loop function is called repeatedly, suspending after
// each call, as long as the function returns a negative value. The index is
// passed to mlua_event_suspend as a deadline index.
int mlua_event_loop(lua_State* ls, MLuaEvent* ev, MLuaEventLoopFn loop,
                    int index);

#if !LIB_MLUA_MOD_MLUA_EVENT
#define mlua_event_require(ls) do {} while(0)
#define mlua_event_can_wait(event) (false)
#define mlua_event_loop(ls, event, loop, index) ((int)0)
#endif

// Start an event handler thread for the given event. The function expects two
// arguments on the stack: the event handler and the cleanup handler. The former
// is called from the thread every time the event is triggered. The latter is
// called when the thread exits. Pops both arguments, pushes the thread, then
// yields to let the thread start and ensure that the cleanup handler
// gets called.
int mlua_event_handle(lua_State* ls, MLuaEvent* event, lua_KFunction cont,
                      lua_KContext ctx);

// Stop the event handler thread for the given event.
void mlua_event_stop_handler(lua_State* ls, MLuaEvent* event);

// Pushes the event handler thread for the given event onto the stack.
int mlua_event_push_handler_thread(lua_State* ls, MLuaEvent* event);

#ifdef __cplusplus
}
#endif

#endif
