# `pico.*` modules

<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

This page describes the modules that wrap the `pico_*` libraries of the
Pico SDK. It only documents the functions that aren't part of the Pico SDK, or
that significantly deviate from the underlying C function. For the others, the
[Pico SDK documentation](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html)
is authoritative.

The test modules can be useful as usage examples.

## `pico`

**Library:** [`pico_base`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_base),
header: [`pico.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_base/include/pico.h),
sources: [`pico_base`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_base)\
**Module:** [`pico`](../lib/pico/pico.c),
build target: `mlua_mod_pico`,
tests: [`pico.test`](../lib/pico/pico.test.lua)

- `board: string`\
  The name of the target board (`PICO_BOARD`).

- `build_type: string`\
  The CMake build type (`Debug`, `Release`, ...).

- `build_target: string`\
  The name of the build target for the program.

- `flash_binary_start: integer`\
  `flash_binary_end, integer`\
  The start and end address of the binary in flash memory.

## `pico.board`

**Library:** [`pico_base`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_base),
headers: [`boards/*.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/boards/include/boards)\
**Module:** `pico.board` (auto-generated),
build target: `mlua_mod_pico_board`,
tests: [`pico.board.test`](../lib/pico/pico.board.test.lua)

This module exposes the constants defined in the
[`boards/*.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/boards/include/boards)
header for the target platform. It is auto-generated.

## `pico.i2c_slave`

**Library:** [`pico_i2c_slave`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_i2c_slave),
header: [`pico/i2c_slave.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_i2c_slave/include/pico/i2c_slave.h),
sources: [`pico_i2c_slave`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_i2c_slave)\
**Module:** [`pico.i2c_slave`](../lib/pico/pico.i2c_slave.c),
build target: `mlua_mod_pico_i2c_slave`,
tests: [`pico.i2c_slave.test`](../lib/pico/pico.i2c_slave.test.lua)

This module implements the same functionality as the `pico_i2c_slave` library,
but with a different API and a different implementation. As of 2023-10, the
implementation of the `pico_i2c_slave` library is flaky: it only works if IRQs
are serviced very quickly, and there are potential race conditions.

- `run(i2c, address, on_receive, on_request) -> Thread`\
  Start an I2C slave as a new thread. The slave uses the provided I2C port and
  replies at the given address. `on_receive` and `on_request` are called when
  data is written to or read from the slave, respectively.

  - `on_receive(i2c, data, first)`\
    The byte `data` was written to the slave. `first` is `true` iff this is the first byte received after the address phase (i.e. the `FIRST_DATA_BYTE` of
    `IC_DATA_CMD` was set).

  - `on_request(i2c)`\
    Data is requested from the slave. The callback should write a byte to the
    TX FIFO with `i2c:write_byte_raw()`.

  The function returns the thread that handles communication for the slave. The
  slave can be shut down by killing the thread.

## `pico.multicore`

**Library:** [`pico_multicore`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_multicore),
header: [`pico/multicore.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore/include/pico/multicore.h),
sources: [`pico_multicore`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore)\
**Module:** [`pico.multicore`](../lib/pico/pico.multicore.c),
build target: `mlua_mod_pico_multicore`,
tests: [`pico.multicore.test`](../lib/pico/pico.multicore.test.lua)

This module allows launching a separate Lua interpreter in core 1. The
interpreters in both cores are independent and don't share state. When resetting
core 1, its interpreter is requested to shut down, and all its resources are
released. Launching and resetting core 1 repeatedly therefore doesn't cause
a memory leak.

- `reset_core1()`\
  If core 1 is running a Lua interpreter, signal that it should terminate, then
  wait for it to do so. Then, reset core 1.

- `launch_core1(module: string, fn = 'main')`\
  Launch a Lua interpreter in core 1, load `module`, then call `module.fn()`.

- `set_shutdown_handler(handler = Thread.shutdown) -> Thread`\
  Start a thread that will call the given function when the core is reset. This
  function must be called in core 1. The shutdown handler can be removed by
  killing the thread or calling the function with a `nil` handler.

### `pico.multicore.fifo`

**Library:** [`pico_multicore_fifo`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#multicore_fifo),
header: [`pico/multicore.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore/include/pico/multicore.h),
sources: [`pico_multicore`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore)\
**Module:** [`pico.multicore.fifo`](../lib/pico/pico.multicore.fifo.c),
build target: `mlua_mod_pico_multicore_fifo`,
tests: [`pico.multicore.fifo.test`](../lib/pico/pico.multicore.fifo.test.lua)

- `ROE: integer = SIO_FIFO_ST_ROE_BITS`\
  `WOF: integer = SIO_FIFO_ST_WOF_BITS`\
  `RDY: integer = SIO_FIFO_ST_RDY_BITS`\
  `VLD: integer = SIO_FIFO_ST_VLD_BITS`\
  Useful constants for `get_status()`.

- `push_blocking(data)` *[yields]*\
  `push_timeout_us(data, timeout) -> boolean` *[yields]*\
  These functions yield if the FIFO is full and threading is enabled. They are
  implemented as busy loops, because there is no IRQ for the `RDY` flag.

- `pop_blocking() -> integer` *[yields]*\
  `pop_timeout_us(timeout) -> integer | nil` *[yields]*\
  These functions yield if the FIFO is empty and the IRQ handler is enabled.

- `enable_irq(enable) -> Thread`\
  [Enable or disable](core.md#irq-enablers) the SIO IRQ handler
  (`SIO_IRQ_PROCx`).

## `pico.platform`

**Library:** [`pico_platform`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_platform),
header: [`pico/platform.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_platform/include/pico/platform.h),
sources: [`pico_platform`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_platform)\
**Module:** [`pico.platform`](../lib/pico/pico.platform.c),
build target: `mlua_mod_pico_platform`,
tests: [`pico.platform.test`](../lib/pico/pico.platform.test.lua)

This module exposes the constants defined in
[`hardware/platform_defs.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/platform_defs.h).

- `busy_wait_at_least_cycles(cycles)`\
  This function doesn't yield.

## `pico.stdio`

**Library:** [`pico_stdio`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio),
header: [`pico/stdio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio/include/pico/stdio.h),
sources: [`pico_stdio`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio)\
**Module:** [`pico.stdio`](../lib/pico/pico.stdio.c),
build target: `mlua_mod_pico_stdio`,
tests: [`pico.stdio.test`](../lib/pico/pico.stdio.test.lua)

- `EOF: integer`

- `getchar() -> integer` *[yields]*\
  `getchar_timeout_us(timeout) -> integer` *[yields]*\
  These functions yield if no data is currently available on `stdin` and
  the "characters available" event is enabled.

- `putchar(c) -> integer`\
  `putchar_raw(c) -> integer`\
  `puts(s) -> integer` \
  `puts_raw(s) -> integer` \
  These functions block without yielding if the output buffer for `stdout` is
  full. This cannot currently be fixed, because the `pico_stdio` library doesn't
  expose functionality for non-blocking output.

- `set_chars_available_callback(fn) -> Thread`\
  Set a handler to be called when characters are available for reading from
  `stdin`. The handler can be removed by killing the thread or calling the
  function with a `nil` handler. This function also enables the "characters
  available" event.

- `read(count) -> string | nil` *[yields]*\
  Read at least one and at most `count` characters from `stdin`. Yields if no
  input is available and the "characters avaible" event is enabled.

- `write(data) -> integer | nil`\
  Write data to `stdout`, and return the number of characters written. This
  function blocks without yielding if the output buffer for `stdout` is full.

- `enable_chars_available(enable)`\
  Enable the "characters available" event if `enable` is true, or disable it
  otherwise.

### `pico.stdio.semihosting`

**Library:** [`pico_stdio_semihosting`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_semihosting),
header: [`pico/stdio_semihosting.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_semihosting/include/pico/stdio_semihosting.h),
sources: [`pico_stdio_semihosting`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_semihosting)\
**Module:** [`pico.stdio.semihosting`](../lib/pico/pico.stdio.semihosting.c)
build target: `mlua_mod_pico_stdio_semihosting`,

- `driver: userdata`\
  The semihosting stdio driver, for use in `pico.stdio` functions.

### `pico.stdio.uart`

**Library:** [`pico_stdio_uart`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_uart),
header: [`pico/stdio_uart.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_uart/include/pico/stdio_uart.h),
sources: [`pico_stdio_uart`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_uart)\
**Module:** [`pico.stdio.uart`](../lib/pico/pico.stdio.uart.c),
build target: `mlua_mod_pico_stdio_uart`,
tests: [`pico.stdio.uart.test`](../lib/pico/pico.stdio.uart.test.lua)

- `driver: userdata`\
  The UART stdio driver, for use in `pico.stdio` functions.

- `init_stdout()`\
  Initialize the UART driver for output only, and add it to the current set of
  stdio drivers.

- `init_stdin()`\
  Initialize the UART driver for input only, and add it to the current set of
  stdio drivers.

### `pico.stdio.usb`

**Library:** [`pico_stdio_usb`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_usb),
header: [`pico/stdio_usb.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_usb/include/pico/stdio_usb.h),
sources: [`pico_stdio_usb`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_usb)\
**Module:** [`pico.stdio.usb`](../lib/pico/pico.stdio.usb.c)
build target: `mlua_mod_pico_stdio_usb`,

- `driver: userdata`\
  The USB stdio driver, for use in `pico.stdio` functions.

## `pico.stdlib`

**Library:** [`pico_stdlib`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_stdlib),
header: [`pico/stdlib.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_stdlib/pico/include/pico/stdlib.h),
sources: [`pico_stdlib`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_stdlib)\
**Module:** [`pico.stdlib`](../lib/pico/pico.stdlib.c),
build target: `mlua_mod_pico_stdlib`,
tests: [`pico.stdlib.test`](../lib/pico/pico.stdlib.test.lua)

- `DEFAULT_LED_PIN_INVERTED: boolean`\
  `DEFAULT_WS2812_PIN: integer | boolean`\
  `DEFAULT_WS2812_POWER_PIN: integer | boolean`

- `check_sys_clock_khz(freq_khz) -> (integer, integer, integer) | nil`

## `pico.time`

**Library:** [`pico_time`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_time),
header: [`pico/time.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_time/include/pico/time.h),
sources: [`pico_time`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_time)\
**Module:** [`pico.time`](../lib/pico/pico.time.c),
build target: `mlua_mod_pico_time`,
tests: [`pico.time.test`](../lib/pico/pico.time.test.lua)

Alarms and repeating timers are implemented using threads and thread timers.
This allows for unlimited timers, without using alarm pools. Alarm pool
functionality is therefore not exposed to Lua and left for use by C code.

- `sleep_until(time)` *[yields]*\
  `sleep_us(duration)` *[yields]*\
  `sleep_ms(duration)` *[yields]*\
  These functions yield until the sleep completes.

- `best_effort_wfe_or_timeout(time) -> boolean`\
  This function blocks with a `WFE` without yielding.

- `add_alarm_at(time, callback, fire_if_past) -> Thread`\
  `add_alarm_in_us(delay_us, callback, fire_if_past) -> Thread`\
  `add_alarm_in_ms(delay_ms, callback, fire_if_past) -> Thread`\
  Add an alarm callback to be called at a specific time or after a delay.

  - `callback(thread) -> integer | nil`\
    The callback to be called when the alarm fires. The return value determines
    if the alarm should be re-scheduled.

    - `<0`: Re-schedule the alarm this many microseconds from the time the alarm
      was previously scheduled to fire.
    - `>0`: Re-schedule the alarm this many microseconds from the time the
      callback returns.
    - `0` or `nil`: Do not re-schedule the alarm.

- `cancel_alarm(thread) -> boolean`\
  Cancel an alarm.

- `add_repeating_timer_us(delay_us, callback) -> Thread`\
  `add_repeating_timer_ms(delay_ms, callback) -> Thread`\
  Add a repeating timer that calls the callback at a specific interval.

  - `callback(thread) -> integer | boolean | nil`\
    The callback to be called when the timer fires. The return value determines
    if the timer should be re-scheduled.

    - `true` or `nil`: Re-schedule the timer, keeping the same interval.
    - `false` or `0`: Do not re-schedule the timer.
    - `integer`: Update the interval and re-schedule the timer.

- `cancel_repeating_timer(thread) -> boolean`\
  Cancel a repeating timer.

## `pico.unique_id`

**Library:** [`pico_unique_id`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_unique_id),
header: [`pico/unique_id.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_unique_id/include/pico/unique_id.h),
sources: [`pico_unique_id`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_unique_id)\
**Module:** [`pico.unique_id`](../lib/pico/pico.unique_id.c),
build target: `mlua_mod_pico_unique_id`,
tests: [`pico.unique_id.test`](../lib/pico/pico.unique_id.test.lua)

- `BOARD_ID_SIZE: integer = PICO_UNIQUE_BOARD_ID_SIZE_BYTES`

- `get_unique_board_id() -> string`\
  Get the unique ID as a binary string.

- `get_unique_board_id_string() -> string`\
  Get the unique ID as a hex string.
