// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/platform.h"

#include <stdbool.h>

#if PICO_ON_DEVICE
#include "hardware/exception.h"
#include "hardware/flash.h"
#endif
#include "hardware/sync.h"
#include "hardware/timer.h"
#include "pico/platform.h"
#include "pico/time.h"

bi_decl(bi_program_feature_group_with_flags(
    MLUA_BI_TAG, MLUA_BI_FROZEN_MODULE, "frozen modules",
    BI_NAMED_GROUP_SORT_ALPHA | BI_NAMED_GROUP_ADVANCED))

__attribute__((noreturn)) void mlua_platform_abort(void) { panic(NULL); }

void mlua_platform_setup_main(int* argc, char* argv[]) {
    *argc = 0;  // The runtime doesn't seem to provide arguments

    // Ensure that the system timer is ticking. This seems to take some time
    // after a reset.
    busy_wait_us(1);
}

#if PICO_ON_DEVICE

// This HardFault exception handler catches semihosting calls when no debugger
// is attached, and makes them silently succeed. For anything else, it does the
// same as the default handler, i.e. hang with a "bkpt 0".
//
// Note that this doesn't work if the target was reset from openocd, even if
// the latter has terminated, because the probe seems to keep watching for and
// handling hardfaults, so semihosting requests halt the core and the handler
// never gets called.
static __attribute__((naked)) void hardfault_handler(void) {
    pico_default_asm_volatile(
        "movs r0, #4\n"         // r0 = SP (from MSP or PSP)
        "mov r1, lr\n"
        "tst r0, r1\n"
        "beq 1f\n"
        "mrs r0, psp\n"
        "b 2f\n"
    "1:\n"
        "mrs r0, msp\n"
    "2:\n"
        "ldr r1, [r0, #24]\n"   // r1 = PC
        "ldrh r2, [r1]\n"       // Check if the instruction is "bkpt 0xab"
        "ldr r3, =0xbeab\n"     // (semihosting)
        "cmp r2, r3\n"
        "beq 3f\n"
        "bkpt 0x00\n"           // Not semihosting, panic
    "3:\n"
        "adds r1, #2\n"         // Semihosting, skip the bkpt instruction
        "str r1, [r0, #24]\n"
        "movs r1, #0\n"         // Set the result of the semihosting call to 0
        "str r1, [r0, #0]\n"
        "bx lr\n"               // Return to the caller
    );
}

void isr_hardfault(void);

#endif  // PICO_ON_DEVICE

void mlua_platform_setup_interpreter(lua_State* ls) {
#if PICO_ON_DEVICE
    // Set the HardFault exception handler if none was set before.
    exception_handler_t handler =
        exception_get_vtable_handler(HARDFAULT_EXCEPTION);
    if (handler == &isr_hardfault) {
        exception_set_exclusive_handler(HARDFAULT_EXCEPTION,
                                        &hardfault_handler);
    }
#endif
}

void mlua_ticks_range(uint64_t* min, uint64_t* max) {
    *min = to_us_since_boot(nil_time);
    *max = to_us_since_boot(at_the_end_of_time);
}

uint64_t mlua_ticks(void) {
    return to_us_since_boot(get_absolute_time());
}

bool mlua_ticks_reached(uint64_t ticks) {
    return time_reached(from_us_since_boot(ticks));
}

bool mlua_wait(uint64_t deadline) {
    if (deadline != to_us_since_boot(at_the_end_of_time)) {
        return best_effort_wfe_or_timeout(from_us_since_boot(deadline));
    }
    __wfe();
    return false;
}

#if PICO_ON_DEVICE

extern char const __flash_binary_start[];
extern char const __flash_binary_end[];

static MLuaFlash const flash = {
    .address = (uintptr_t)__flash_binary_start,
    .size = PICO_FLASH_SIZE_BYTES,
    .write_size = FLASH_PAGE_SIZE,
    .erase_size = FLASH_SECTOR_SIZE,
};

MLuaFlash const* mlua_platform_flash(void) { return &flash; }

uintptr_t mlua_platform_binary_size(void) {
    return __flash_binary_end - __flash_binary_start;
}

#else  // !PICO_ON_DEVICE

MLuaFlash const* mlua_platform_flash(void) { return NULL; }
uintptr_t mlua_platform_binary_size(void) { return 0; }

#endif  // !PICO_ON_DEVICE
