// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_EVENT_COMMON_H
#define _MLUA_LIB_COMMON_EVENT_COMMON_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Require the mlua.event module.
void mlua_event_require(lua_State* ls);

// Register the current thread to be notified when an event triggers.
void mlua_event_watch(lua_State* ls, MLuaEvent const* ev);

// Unregister the current thread from notifications for an event.
void mlua_event_unwatch(lua_State* ls, MLuaEvent const* ev);

// Resume the watcher of an event. "resume" is the index where Thread.resume
// can be found.
bool mlua_event_resume_watcher(lua_State* ls, MLuaEvent const* ev, int resume);

// Remove the watcher of an event.
void mlua_event_remove_watcher(lua_State* ls, MLuaEvent const* ev);

// Yield from the running thread.
static inline int mlua_event_yield(lua_State* ls, int nresults,
                                   lua_KFunction cont, lua_KContext ctx) {
    lua_yieldk(ls, nresults, ctx, cont);
    return cont(ls, LUA_OK, ctx);
}

// Suspend the running thread. If the given index is non-zero, yield the value
// at that index, which must be a deadline in microseconds. Otherwise, yield
// false to suspend indefinitely.
int mlua_event_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                       int index);

typedef int (*MLuaEventLoopFn)(lua_State*, bool);

// Return true iff yielding is enabled.
bool mlua_yield_enabled(lua_State* ls);

#if !LIB_MLUA_MOD_MLUA_EVENT
// TODO: Allow force-enabling yielding => eliminate blocking code
#define mlua_yield_enabled(ls) (false)
#endif

// Return true iff waiting for the given event is possible, i.e. yielding is
// enabled and the event is enabled.
bool mlua_event_can_wait(lua_State* ls, MLuaEvent const* ev);

// Run an event loop. The loop function is called repeatedly, suspending after
// each call, as long as the function returns a negative value. The index is
// passed to mlua_event_suspend as a deadline index.
int mlua_event_loop(lua_State* ls, MLuaEvent const* ev, MLuaEventLoopFn loop,
                    int index);

#if !LIB_MLUA_MOD_MLUA_EVENT
#define mlua_event_require(ls) do {} while(0)
#define mlua_event_can_wait(ls, event) (false)
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
void mlua_event_stop_handler(lua_State* ls, MLuaEvent const* event);

// Pushes the event handler thread for the given event onto the stack.
int mlua_event_push_handler_thread(lua_State* ls, MLuaEvent const* event);

#ifdef __cplusplus
}
#endif

#endif
