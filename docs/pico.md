<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

# `pico.*` modules

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

- `flash_binary_start: pointer`\
  `flash_binary_end, pointer`\
  Pointers to the start and end of the binary in flash memory.

- `OK: integer`\
  `ERROR_*: integer`\
  Common error codes for the Pico SDK.

- `SDK_VERSION_*: integer`\
  `SDK_VERSION_STRING: string`\
  The SDK major and minor version and the revision of the Pico SDK.

- `DEFAULT_*: integer`\
  `FLASH_*: integer`\
  `SMPS_MODE_PIN: integer`\
  `CYW43_WL_GPIO_COUNT: integer`\
  `CYW43_WL_GPIO_LED_PIN: integer`\
  Per-board defaults and constants, as defined in the board-specific headers.

- `error_str(err) -> string`\
  Return a description of the given `pico.ERROR_*` error code.

- `xip_ctr(clear = false) -> (hit, acc)`\
  Return the number of hits and the total number of accesses to the XIP cache.
  When `clear` is `true`, clear the counters after reading them.

## `pico.board`

**Library:** [`pico_base`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_base),
headers: [`boards/*.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/boards/include/boards)\
**Module:** `pico.board` (auto-generated),
build target: `mlua_mod_pico.board`,
tests: [`pico.board.test`](../lib/pico/pico.board.test.lua)

This module exposes the constants defined in the
[`boards/*.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/boards/include/boards)
header for the target board. It is auto-generated.

## `pico.bootrom`

**Library:** [`pico_bootrom`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_bootrom),
header: [`pico/bootrom.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_bootrom/include/pico/bootrom.h),
sources: [`pico_bootrom`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_bootrom)\
**Module:** [`pico.bootrom`](../lib/pico/pico.bootrom.c),
build target: `mlua_mod_pico.bootrom`,
tests: [`pico.bootrom.test`](../lib/pico/pico.bootrom.test.lua)

- `START: pointer = pointer(0x00000000)`\
  `SIZE: integer = 0x40000`\
  A pointer to the bootrom, and its size.

- `VERSION: integer`\
  The version of the bootrom.

- `FUNC_*: integer`\
  Lookup codes for various bootrom functions.

- `INTF_MASS_STORAGE: integer`\
  `INTF_PICOBOOT: integer`\
  Interfaces that can be disabled in `reset_usb_boot()`.

- `table_code(id) -> integer`\
  Return a lookup code for a two-character identifier.

- `func_lookup([code, ...]) -> pointer`\
  Look up bootrom functions by code.

- `data_lookup([code, ...]) -> pointer`\
  Look up bootrom addresses by code.

- `reset_usb_boot(pin_mask, disable_interface_mask)`\
  Reboot the device into BOOTSEL mode. `pin_mask` is either 0 or a single bit
  set for the GPIO to use to indicate activity. `disable_interface_mask` is a
  bit mask of interfaces to disable, an OR of `INTF_*` values.

## `pico.cyw43`

**Library:** [`pico_cyw43_driver`](https://www.raspberrypi.com/documentation/pico-sdk/networking.html#pico_cyw43_driver),
header: [`pico/cyw43_driver.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_cyw43_driver/include/pico/cyw43_driver.h),
sources: [`pico_cyw43_driver`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_cyw43_driver),
[`cyw43-driver`](https://github.com/georgerobotics/cyw43-driver/tree/main/src)\
**Module:** [`pico.cyw43`](../lib/pico/pico.cyw43.c),
build target: `mlua_mod_pico.cyw43`

This module exposes the driver for the CYW43 on the Pico W. Errors returned by
the functions in this module correspond to `pico.ERROR_*` constants.

- `VERSION: integer`\
  The driver version.

- `TRACE_*: integer`\
  Trace flags.

- `LINK_*: integer`\
  Link statuses.

- `WPA_AUTH_PSK: integer`\
  `WPA2_AUTH_PSK: integer`\
  STA and AP auth settings.

- `AUTH_*: integer`\
  Authorization types.

- `*_POWERSAVE_MODE: integer`\
  Power save modes.

- `CHANNEL_NONE: integer`\
  Value to use when not selecting a particular channel.

- `ITF_STA: integer`\
  `ITF_AP: integer`\
  Network interface types for station and access point modes, respectively.

- `COUNTRY_*: integer`\
  Country codes.

- `init() -> boolean`\
  Initialize the driver. Returns `true` iff initialization succeeded.

- `deinit()`\
  De-initializes the driver.

- `is_initialized() -> boolean`\
  Return `true` iff the driver has been initialized.

- `ioctl(cmd, data, itf) -> string | (fail, err)`\
  Issue a control command to the CYW43. Returns the (possibly modified) data.

- `tcpip_link_status() -> integer`\
  Return the link status, a superset of the Wi-Fi link status, as one of the
  `LINK_*` values.

- `link_status_str(status) -> string`\
  Return a string describing the given link status.

- `gpio_set(num, value)`\
  Set a GPIO on the CYW43.

- `gpio_get(num) -> boolean`\
  Get the state of a GPIO on the CYW43.

## `pico.cyw43.util`

**Module:** [`pico.cyw43.util`](../lib/pico/pico.cyw43.util.lua),
build target: `mlua_mod_pico.cyw43.util`

This module provides helper functions to work with the CYW43.

- `wifi_connect(ssid, password, auth, deadline = nil) -> true | (nil, msg)`\
  Connect to a Wi-Fi network on the `ITF_STA` interface, and wait for the link
  status to become `LINK_UP` or the connection to fail with a permanent error.
  The deadline is an [absolute time](mlua.md#absolute-time).

## `pico.cyw43.wifi`

**Library:** sources: [`cyw43-driver`](https://github.com/georgerobotics/cyw43-driver/tree/main/src)\
**Module:** [`pico.cyw43.wifi`](../lib/pico/pico.cyw43.wifi.c),
build target: `mlua_mod_pico.cyw43.wifi`

This module exposes the Wi-Fi functionality of the CYW43 driver. Errors returned
by the functions in this module correspond to `pico.ERROR_*` constants.

- `SCAN_ACTIVE: integer`\
  `SCAN_PASSIVE: integer`\
  Scan types for active and passive scanning, respectively.

- `*_PM: integer`\
  Predefined power management values.

- `set_up(itf, up, country)`\
  Turn on Wi-Fi and set the country for regulation purposes. `itf` is either
  `ITF_STA` for the station interface or `ITF_AP` for the access point
  interface.

- `pm(value) -> true | (fail, err)`\
  Set the Wi-Fi power management mode.

- `get_pm() -> integer | (fail, err)`\
  Return the current Wi-Fi power management mode.

- `pm_value(pm_mode, pm2_sleep_ret_ms, li_beacon_period, li_dtim_period, li_assoc) -> integer`\
  Compute a power management mode from power management parameters.

- `link_status(itf) -> integer`\
  Return the Wi-Fi link status of an interface, as one of the `cyw43.LINK_*`
  values.

- `get_mac(itf) -> string | (fail, err)`\
  Return the MAC address of an interface.

- `update_multicast_filter(address, add) -> true | (fail, err)`\
  Add (`add = true`) or remove (`add = false`) a multicast group address.

- `scan(opts = nil, queue = 8) -> iterator`\
  Perform a scan for Wi-Fi networks. The scan options `opts` are a table with
  keys `ssid` (when present, scan only networks with that SSID), and `scan_type`
  (one of the `SCAN_*` constants). The iterator produces `(result, dropped)`
  values:

  - `result`: A table with keys: `ssid`, `bssid`, `rssi`, `channel` and
    `auth_mode`.
  - `dropped`: The number of results that had to be dropped because the result
    queue was full. If this is non-zero, the size of the queue (`queue`) should
    be increased.

- `scan_active() -> boolean`\
  Return true iff a scan is currently in progress.

- `join(ssid, key = nil, auth = cyw43.AUTH_OPEN, bssid = nil, channel = cyw43.CHANNEL_NONE) -> true | (fail, err)`\
  Initiate a connection to a Wi-Fi network on the `ITF_STA` interface. The
  connection is established in the background, and its status can be polled with
  `link_status()` or `cyw43.tcpip_link_status()`.

- `leave(itf) -> true | (fail, err)`\
  Disassociate an interface from a Wi-Fi network.

- `get_rssi() -> integer | (fail, err)`\
  Return the signal strength (RSSI) of the Wi-Fi network.

- `get_bssid() -> string | (fail, err)`\
  Return the BSSID of the connected Wi-Fi network.

## `pico.i2c_slave`

**Library:** [`pico_i2c_slave`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_i2c_slave),
header: [`pico/i2c_slave.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_i2c_slave/include/pico/i2c_slave.h),
sources: [`pico_i2c_slave`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_i2c_slave)\
**Module:** [`pico.i2c_slave`](../lib/pico/pico.i2c_slave.c),
build target: `mlua_mod_pico.i2c_slave`,
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
build target: `mlua_mod_pico.multicore`,
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

## `pico.multicore.fifo`

**Library:** [`pico_multicore_fifo`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#multicore_fifo),
header: [`pico/multicore.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore/include/pico/multicore.h),
sources: [`pico_multicore`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_multicore)\
**Module:** [`pico.multicore.fifo`](../lib/pico/pico.multicore.fifo.c),
build target: `mlua_mod_pico.multicore.fifo`,
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
build target: `mlua_mod_pico.platform`,
tests: [`pico.platform.test`](../lib/pico/pico.platform.test.lua)

This module exposes the constants defined in
[`hardware/platform_defs.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/platform_defs.h).

- `busy_wait_at_least_cycles(cycles)`\
  This function doesn't yield.

## `pico.rand`

**Library:** [`pico_rand`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_rand),
header: [`pico/rand.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_rand/include/pico/rand.h),
sources: [`pico_rand`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_rand)\
**Module:** [`pico.rand`](../lib/pico/pico.rand.c),
build target: `mlua_mod_pico.rand`,
tests: [`pico.rand.test`](../lib/pico/pico.rand.test.lua)

- `get_rand_32() -> integer`\
  Generate a random 32-bit value.

- `get_rand_64() -> Int64`\
  Generate a random 64-bit value.

- `get_rand_128() -> (Int64, Int64)`\
  Generate a random 128-bit value, as a pair of 64-bit values.

## `pico.standard_link`

**Library:** [`pico_standard_link`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_standard_link),
sources: [`pico_standard_link`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_standard_link)\
**Module:** [`pico.standard_link`](../lib/pico/pico.standard_link.c),
build target: `mlua_mod_pico.standard_link`,
tests: [`pico.standard_link.test`](../lib/pico/pico.standard_link.test.lua)

This module exposes symbols defined in the linker table.

- `FLASH: pointer = pointer(0x10000000)`\
  `FLASH_SIZE: integer = 0x200000`\
  `RAM: pointer = pointer(0x20000000)`\
  `RAM_SIZE: integer = 0x40000`\
  `SCRATCH_X: pointer = pointer(0x20040000)`\
  `SCRATCH_X_SIZE: integer = 0x1000`\
  `SCRATCH_Y: pointer = pointer(0x20041000)`\
  `SCRATCH_Y_SIZE: integer = 0x1000`\
  Pointers to, and sizes of the memory areas, as used by the linker.

- `flash_binary_start: pointer = __flash_binary_start`\
  `flash_binary_end: pointer = __flash_binary_end`\
  The start and end of the binary in `FLASH`.

- `logical_binary_start: pointer = __logical_binary_start`\
  The logical start of the binary, i.e. the part after the second stage
  bootloader.

- `binary_info_header_end: pointer = __binary_info_header_end`\
  The end of the binary info header.

- `binary_info_start: pointer = __binary_info_start`\
  `binary_info_end: pointer = __binary_info_end`\
  The start and end of the binary info.

- `ram_vector_table: pointer = ram_vector_table`\
  The address of the vector table in RAM.

- `data_source: pointer = __etext`\
  `data_start: pointer = __data_start__`\
  `data_end: pointer = __data_end__`\
  The source (in `FLASH`), start and end (in `RAM`) of initialized data.

- `bss_start: pointer = __bss_start__`\
  `bss_end: pointer = __bss_end__`\
  The start and end of zero-initialized data.

- `scratch_x_source: pointer = __scratch_x_source__`\
  `scratch_x_start: pointer = __scratch_x_start__`\
  `scratch_x_end: pointer = __scratch_x_end__`\
  The source (in `FLASH`), start and end (in `SCRATCH_X`) of data allocated in
  scratch X.

- `scratch_y_source: pointer = __scratch_y_source__`\
  `scratch_y_start: pointer = __scratch_y_start__`\
  `scratch_y_end: pointer = __scratch_y_end__`\
  The source (in `FLASH`), start and end (in `SCRATCH_Y`) of data allocated in
  scratch Y.

- `heap_start: pointer = __end__`\
  `heap_limit: pointer = __HeapLimit`\
  `heap_end: pointer = __StackLimit`\
  The start of the heap, its guaranteed limit (as set by `PICO_HEAP_SIZE`) and
  its end.

- `stack1_bottom: pointer = __StackOneBottom`\
  `stack1_top: pointer = __StackOneTop`\
  The bottom and top of the stack for core 1. The stack size is set by
  `PICO_CORE1_STACK_SIZE`.

- `stack_bottom: pointer = __StackBottom`\
  `stack_top: pointer = __StackTop`\
  The bottom and top of the stack for core 0. The stack size is set by
  `PICO_STACK_SIZE`.

## `pico.stdio`

**Library:** [`pico_stdio`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio),
header: [`pico/stdio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio/include/pico/stdio.h),
sources: [`pico_stdio`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio)\
**Module:** [`pico.stdio`](../lib/pico/pico.stdio.c),
build target: `mlua_mod_pico.stdio`,
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

## `pico.stdio.semihosting`

**Library:** [`pico_stdio_semihosting`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_semihosting),
header: [`pico/stdio_semihosting.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_semihosting/include/pico/stdio_semihosting.h),
sources: [`pico_stdio_semihosting`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_semihosting)\
**Module:** [`pico.stdio.semihosting`](../lib/pico/pico.stdio.semihosting.c)
build target: `mlua_mod_pico.stdio.semihosting`,
tests: [`pico.stdio.semihosting.test`](../lib/pico/pico.stdio.semihosting.test.lua)

- `driver: userdata`\
  The semihosting stdio driver, for use in `pico.stdio` functions.

## `pico.stdio.uart`

**Library:** [`pico_stdio_uart`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_uart),
header: [`pico/stdio_uart.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_uart/include/pico/stdio_uart.h),
sources: [`pico_stdio_uart`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_uart)\
**Module:** [`pico.stdio.uart`](../lib/pico/pico.stdio.uart.c),
build target: `mlua_mod_pico.stdio.uart`,
tests: [`pico.stdio.uart.test`](../lib/pico/pico.stdio.uart.test.lua)

- `driver: userdata`\
  The UART stdio driver, for use in `pico.stdio` functions.

- `init_stdout()`\
  Initialize the UART driver for output only, and add it to the current set of
  stdio drivers.

- `init_stdin()`\
  Initialize the UART driver for input only, and add it to the current set of
  stdio drivers.

## `pico.stdio.usb`

**Library:** [`pico_stdio_usb`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_usb),
header: [`pico/stdio_usb.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_usb/include/pico/stdio_usb.h),
sources: [`pico_stdio_usb`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/pico_stdio_usb)\
**Module:** [`pico.stdio.usb`](../lib/pico/pico.stdio.usb.c)
build target: `mlua_mod_pico.stdio.usb`,
tests: [`pico.stdio.usb.test`](../lib/pico/pico.stdio.usb.test.lua)

- `driver: userdata`\
  The USB stdio driver, for use in `pico.stdio` functions.

## `pico.stdlib`

**Library:** [`pico_stdlib`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_stdlib),
header: [`pico/stdlib.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_stdlib/pico/include/pico/stdlib.h),
sources: [`pico_stdlib`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_stdlib)\
**Module:** [`pico.stdlib`](../lib/pico/pico.stdlib.c),
build target: `mlua_mod_pico.stdlib`,
tests: [`pico.stdlib.test`](../lib/pico/pico.stdlib.test.lua)

- `DEFAULT_LED_PIN_INVERTED: boolean`\
  `DEFAULT_WS2812_PIN: integer | boolean`\
  `DEFAULT_WS2812_POWER_PIN: integer | boolean`

## `pico.time`

**Library:** [`pico_time`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_time),
header: [`pico/time.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_time/include/pico/time.h),
sources: [`pico_time`](https://github.com/raspberrypi/pico-sdk/blob/master/src/common/pico_time)\
**Module:** [`pico.time`](../lib/pico/pico.time.c),
build target: `mlua_mod_pico.time`,
tests: [`pico.time.test`](../lib/pico/pico.time.test.lua)

Alarms and repeating timers are implemented using threads and thread timers.
This allows for unlimited timers, without using alarm pools. Alarm pool
functionality is therefore not exposed to Lua and left for use by C code.

- `nil_time: Int64`\
  `at_the_end_of_time: Int64`\
  The minimum and maximum values returned by `get_absolute_time()`,
  respectively.

- `get_absolute_time() -> Int64`\
  Return the current [absolute time](mlua.md#absolute-time).

- `get_absolute_time_int() -> integer`\
  Return the low-order bits of the current
  [absolute time](mlua.md#absolute-time) that fit a Lua integer.

- `to_ms_since_boot(time) -> integer`\
  Convert an [absolute time](mlua.md#absolute-time) into a number of
  milliseconds since boot.

- `delayed_by_us(time, delay) -> Int64`\
  Return an [absolute time](mlua.md#absolute-time) calculated by adding a
  microsecond delay to the given time. `delay` is interpreted as a `uint64_t`.

- `delayed_by_ms(time, delay) -> Int64`\
  Return an [absolute time](mlua.md#absolute-time) calculated by adding a
  millisecond delay to the given time. `delay` is interpreted as a `uint32_t`.

- `make_timeout_time_us(delay) -> integer | Int64`\
  Return an [absolute time](mlua.md#absolute-time) that is `delay` microseconds
  in the future. `delay` is interpreted as a `uint64_t`.

- `make_timeout_time_ms(delay) -> integer | Int64`\
  Return an [absolute time](mlua.md#absolute-time) that is `delay` milliseconds
  in the future. `delay` is interpreted as an unsigned integer.

- `absolute_time_diff_us(from, to) -> integer | Int64`\
  Return the difference in microseconds between the
  [absolute times](mlua.md#absolute-time) `from` and `to`.

- `absolute_time_min(a, b) -> Int64`\
  Return the earlier of two [absolute times](mlua.md#absolute-time).

- `is_at_the_end_of_time(time) -> boolean`\
  `is_nil_time(time) -> boolean`\
  Return `true` iff the given [absolute time](mlua.md#absolute-time) is equal to
  `at_the_end_of_time` or `nil_time`, respectively.

- `sleep_until(time)` *[yields]*\
  Suspend the current thread until the given
  [absolute time](mlua.md#absolute-time) is reached.

- `sleep_us(duration)` *[yields]*\
  `sleep_ms(duration)` *[yields]*\
  Suspend the current thread for the given duration.

- `best_effort_wfe_or_timeout(time) -> boolean`\
  Block with a `WFE` instruction, at most until the given
  [absolute time](mlua.md#absolute-time) is reached.

- `add_alarm_at(time, callback, fire_if_past) -> Thread`\
  `add_alarm_in_us(delay, callback, fire_if_past) -> Thread`\
  `add_alarm_in_ms(delay, callback, fire_if_past) -> Thread`\
  Add an alarm callback to be called at a specific time or after a delay.
  Returns the [event handler thread](core.md#callbacks).

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

- `add_repeating_timer_us(delay, callback) -> Thread`\
  `add_repeating_timer_ms(delay, callback) -> Thread`\
  Add a repeating timer that calls the callback at a specific interval.
  Returns the [event handler thread](core.md#callbacks).

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
build target: `mlua_mod_pico.unique_id`,
tests: [`pico.unique_id.test`](../lib/pico/pico.unique_id.test.lua)

- `BOARD_ID_SIZE: integer = PICO_UNIQUE_BOARD_ID_SIZE_BYTES`

- `get_unique_board_id() -> string`\
  Get the unique ID as a binary string.

- `get_unique_board_id_string() -> string`\
  Get the unique ID as a hex string.
