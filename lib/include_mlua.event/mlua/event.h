#ifndef _MLUA_LIB_EVENT_H
#define _MLUA_LIB_EVENT_H

#include <stdbool.h>
#include <stdint.h>

#include "pico/types.h"

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MLUA_BLOCK_WHEN_NON_YIELDABLE
#define MLUA_BLOCK_WHEN_NON_YIELDABLE 0
#endif

// An event number.
typedef uint8_t Event;

// Claim an event.
void mlua_event_claim(lua_State* ls, Event* pev);

// Free an event.
void mlua_event_unclaim(lua_State* ls, Event* pev);

// Enable or disable IRQ handling. The argument at the given index determines
// what is done:
//  - true, nil, none, integer: Claim an event, set the IRQ handler and enable
//    the IRQ. If the argument is a non-negative integer, add a shared handler
//    with the given priority. Otherwise, set an exclusive handler.
//  - false: Disable the IRQ, remove the IRQ handler and unclaim the event.
void mlua_event_enable_irq(lua_State* ls, Event* ev, uint irq,
                           void (*handler)(void), int index,
                           lua_Integer priority);

// Set the pending state of an event.
void mlua_event_set(Event ev, bool pending);

// Register the current thread to be notified when an event triggers.
void mlua_event_watch(lua_State* ls, Event ev);

// Unregister the current thread from notifications for an event.
void mlua_event_unwatch(lua_State* ls, Event ev);

// Suspend the running thread, by yielding true.
int mlua_event_suspend(lua_State* ls, lua_KFunction cont);

#ifdef __cplusplus
}
#endif

#endif