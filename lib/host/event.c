// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/thread.h"

#include "mlua/module.h"
#include "mlua/platform.h"

void mlua_event_dispatch(lua_State* ls, uint64_t deadline) {
    bool wake = deadline == MLUA_TICKS_MIN;
#if MLUA_THREAD_STATS
    MLuaGlobal* g = mlua_global(ls);
#endif
    for (;;) {
#if MLUA_THREAD_STATS
        ++g->thread_dispatches;
#endif

        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || mlua_ticks64_reached(deadline)) return;
        wake = false;

        // Wait for events, up to the deadline.
#if MLUA_THREAD_STATS
        ++g->thread_waits;
#endif
        mlua_wait(deadline);
    }
}
