// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/event.h"

#include "mlua/platform.h"

int mlua_event_dispatch(lua_State* ls, uint64_t deadline, int resume) {
    uint64_t ticks_min, ticks_max;
    mlua_ticks_range(&ticks_min, &ticks_max);
    bool wake = deadline == ticks_min || mlua_ticks_reached(deadline);

    for (;;) {
        // Return if at least one thread was resumed or the deadline has passed.
        if (wake || mlua_ticks_reached(deadline)) break;
        wake = false;

        // Wait for events, up to the deadline.
        mlua_wait(deadline);
    }
    return 0;
}
