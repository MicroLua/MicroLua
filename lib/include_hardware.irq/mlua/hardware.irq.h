#ifndef _MLUA_LIB_HARDWARE_IRQ_H
#define _MLUA_LIB_HARDWARE_IRQ_H

#include <stdint.h>
#include <stdbool.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef __cplusplus
extern "C" {
#endif

// Set an IRQ handler and a signal handler for the given IRQ. If handler_wrapper
// is non-NULL, use it as the signal handler, passing the original handler as
// the first upvalue.
void mlua_irq_set_handler(lua_State* ls, uint irq, void (*irq_handler)(void),
                          lua_CFunction handler_wrapper, int handler);

// Set the pending state of the signal for the given IRQ.
void mlua_irq_set_signal(uint irq, bool pending);

#ifdef __cplusplus
}
#endif

#endif
