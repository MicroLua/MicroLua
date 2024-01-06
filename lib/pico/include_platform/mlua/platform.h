// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PLATFORM_H
#define _MLUA_LIB_PICO_PLATFORM_H

#include <stdbool.h>
#include <stdint.h>

#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/binary_info.h"
#include "pico/platform.h"
#include "pico/time.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/platform_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MLUA_HASH_SYMBOL_TABLES_DEFAULT 1

#define MLUA_BI_TAG BINARY_INFO_MAKE_TAG('M', 'L')
#define MLUA_BI_FROZEN_MODULE 0xcb9305cf

#define MLUA_PLATFORM_REGISTER_MODULE(n) \
    bi_decl(bi_string(MLUA_BI_TAG, MLUA_BI_FROZEN_MODULE, #n))

// Abort the program.
__attribute__((__noreturn__))
static inline void mlua_platform_abort(void) { panic(NULL); }

// Perform set up at the very beginning of main().
static inline void mlua_platform_setup_main(int* argc, char* argv[]) {
    // When crt0.S calls main() in platform_entry, it doesn't explicitly set r0,
    // and the value of argc may be undefined. We fix that here.
    *argc = 0;

    // Ensure that the system timer is ticking. This seems to take some time
    // after a reset.
    busy_wait_us(1);
}

// Perform set up after creating an interpreter.
void mlua_platform_setup_interpreter(lua_State* ls);

// Return the range of values that can be returned by mlua_ticks().
static inline void mlua_ticks_range(uint64_t* min, uint64_t* max) {
    *min = to_us_since_boot(nil_time);
    *max = to_us_since_boot(at_the_end_of_time);
}

// Return the current microsecond ticks, as given by a monotonic clock.
static inline uint64_t mlua_ticks(void) {
    return to_us_since_boot(get_absolute_time());
}

// Return true iff the given ticks value has been reached.
static inline bool mlua_ticks_reached(uint64_t ticks) {
    return time_reached(from_us_since_boot(ticks));
}

// Wait for an event, up to the given deadline. Returns true iff the deadline
// was reached.
static inline bool mlua_wait(uint64_t deadline) {
    if (deadline != to_us_since_boot(at_the_end_of_time)) {
        return best_effort_wfe_or_timeout(from_us_since_boot(deadline));
    }
    __wfe();
    return false;
}

#if PICO_ON_DEVICE

// Return a description of the flash memory of the platform, or NULL if the
// platform doesn't have flash memory.
MLuaFlash const* mlua_platform_flash(void);

// Return the size of the binary.
static inline uintptr_t mlua_platform_binary_size(void) {
    extern char const __flash_binary_start[];
    extern char const __flash_binary_end[];
    return __flash_binary_end - __flash_binary_start;
}

#else  // !PICO_ON_DEVICE

static inline MLuaFlash const* mlua_platform_flash(void) { return NULL; }
static inline uintptr_t mlua_platform_binary_size(void) { return 0; }

#endif  // !PICO_ON_DEVICE

#ifdef __cplusplus
}
#endif

#endif
