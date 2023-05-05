#ifndef _MLUA_LIB_SIGNAL_H
#define _MLUA_LIB_SIGNAL_H

#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// A signal number.
typedef signed char SigNum;

// Claim a signal and set its handler to the value at the given index in the
// stack.
SigNum mlua_signal_claim(lua_State* ls, int handler);

// Free a signal.
void mlua_signal_unclaim(lua_State* ls, SigNum sig);

// Modify the handler for a signal.
void mlua_signal_set_handler(lua_State* ls, SigNum sig, int handler);

// Set the pending state of a signal.
void mlua_signal_set(SigNum sig, bool pending);

#ifdef __cplusplus
}
#endif

#endif
