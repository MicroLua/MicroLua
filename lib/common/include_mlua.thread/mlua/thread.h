// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_COMMON_MLUA_THREAD_H
#define _MLUA_LIB_COMMON_MLUA_THREAD_H

#include <stdbool.h>
#include <stdint.h>

#include "lua.h"
#include "lauxlib.h"

#include "mlua/event.h"
#include "mlua/util.h"

#ifdef __cplusplus
extern "C" {
#endif

// Require the mlua.thread module.
void mlua_thread_require(lua_State* ls);

// Define a symbol that resolves to a function if the mlua.thread module is
// available, or to false otherwise.
#if LIB_MLUA_MOD_MLUA_THREAD
#define MLUA_SYM_F_THREAD(n, p) MLUA_SYM_F(n, p)
#else
#define MLUA_SYM_F_THREAD(n, p) MLUA_SYM_V(n, boolean, false)
#endif

#if !LIB_MLUA_MOD_MLUA_THREAD
#define mlua_thread_require(ls) do {} while(0)
#endif

// Return true iff blocking event handling is selected.
bool mlua_thread_blocking(lua_State* ls);

#if !LIB_MLUA_MOD_MLUA_THREAD
#define mlua_thread_blocking(ls) (1)
#endif

// Yield from the running thread.
static inline int mlua_thread_yield(lua_State* ls, int nresults,
                                   lua_KFunction cont, lua_KContext ctx) {
    lua_yieldk(ls, nresults, ctx, cont);
    return cont(ls, LUA_OK, ctx);
}

// Suspend the running thread. If the given index is non-zero, yield the value
// at that index, which must be a deadline in microseconds. Otherwise, yield
// nil to suspend indefinitely.
int mlua_thread_suspend(lua_State* ls, lua_KFunction cont, lua_KContext ctx,
                        int index);

// Return the given argument as a thread. Raises an error if the argument is not
// a thread.
lua_State* mlua_check_thread(lua_State* ls, int arg);

// Push the thread metatable field with the given name. Returns the type of the
// field, or LUA_TNIL if the metatable doesn't have this field.
int mlua_thread_meta(lua_State* ls, char const* name);

// Start a new thread calling the function at the top of the stack. Pops the
// function from the stack and pushes the thread.
void mlua_thread_start(lua_State* ls);

// Kill the thread at the top of the stack. Pops the thread from the stack, and
// pushes a boolean indicating if the thread was alive.
void mlua_thread_kill(lua_State* ls);

// Prepare an event pointer for multi-event operations. The pointer is updated
// to the first event in the array for which the mask has a bit set. Returns the
// mask value to use in the multi-event operations; it corresponds to the mask
// of events following the first one.
static inline unsigned int mlua_event_multi(MLuaEvent const** evs,
                                            unsigned int mask) {
    unsigned int bit = MLUA_CTZ(mask);
    *evs += bit;
    return mask >> (bit + 1);
}

// Resume the watcher of an event. "resume" is the index where Thread.resume
// can be found.
bool mlua_event_resume_watcher(lua_State* ls, MLuaEvent const* ev);

// Remove the watcher of an event.
void mlua_event_remove_watcher(lua_State* ls, MLuaEvent const* ev);

// Return true iff waiting for the given events is possible, i.e. non-blocking
// event handling is selected and the events are enabled.
bool mlua_event_can_wait(lua_State* ls, MLuaEvent const* evs,
                         unsigned int mask);

#if !LIB_MLUA_MOD_MLUA_THREAD
#define mlua_event_can_wait(ls, evs, mask) (0)
#endif

typedef int (*MLuaEventLoopFn)(lua_State*, bool);

// Wait for a condition to be fulfilled. The loop function is called repeatedly,
// suspending after each call, as long as the function returns a negative value.
// The index is passed to mlua_thread_suspend() as a deadline index.
int mlua_event_wait(lua_State* ls, MLuaEvent const* evs,
                    unsigned int mask, MLuaEventLoopFn loop, int index);

#if !LIB_MLUA_MOD_MLUA_THREAD
#define mlua_event_wait(ls, evs, mask, loop, index) ((int)0)
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
