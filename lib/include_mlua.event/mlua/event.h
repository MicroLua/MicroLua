#ifndef _MLUA_LIB_EVENT_H
#define _MLUA_LIB_EVENT_H

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

// An event identifier.
typedef uint8_t MLuaEvent;

// Claim an event.
void mlua_event_claim(lua_State* ls, MLuaEvent* pev);

// Free an event.
void mlua_event_unclaim(lua_State* ls, MLuaEvent* pev);

// Lock event handling. This disables interrupts.
__force_inline static uint32_t mlua_event_lock(void) {
    return save_and_disable_interrupts();
}

// Unlock event handling.
__force_inline static void mlua_event_unlock(uint32_t save) {
    restore_interrupts(save);
}

// Parse the enable_irq argument.
bool mlua_event_enable_irq_arg(lua_State* ls, int index, lua_Integer* priority);

// Set an IRQ handler.
void mlua_event_set_irq_handler(uint irq, irq_handler_t handler,
                                lua_Integer priority);

// Remove an IRQ handler.
void mlua_event_remove_irq_handler(uint irq, irq_handler_t handler);

// Enable or disable IRQ handling. The argument at the given index determines
// what is done:
//  - true, nil, none, integer: Claim an event, set the IRQ handler and enable
//    the IRQ. If the argument is a non-negative integer, add a shared handler
//    with the given priority. Otherwise, set an exclusive handler.
//  - false: Disable the IRQ, remove the IRQ handler and unclaim the event.
void mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                           irq_handler_t handler, int index,
                           lua_Integer priority);

// Set the pending state of an event.
void mlua_event_set(MLuaEvent ev, bool pending);

// Register the current thread to be notified when an event triggers.
void mlua_event_watch(lua_State* ls, MLuaEvent ev);

// Unregister the current thread from notifications for an event.
void mlua_event_unwatch(lua_State* ls, MLuaEvent ev);

// Yield from the running thread.
int mlua_event_yield(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                     int nresults);

// Suspend the running thread. If the given index is non-zero, yield the value
// at that index, which must be a deadline in microseconds. Otherwise, yield
// false to suspend indefinitely.
int mlua_event_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                       int index);

// Wait for an event, suspending as long as try_get returns a negative value.
// The index is passed to mlua_event_suspend as a deadline index.
int mlua_event_wait(lua_State* ls, MLuaEvent event, lua_CFunction try_get,
                    int index);

#ifdef __cplusplus
}
#endif

#endif
