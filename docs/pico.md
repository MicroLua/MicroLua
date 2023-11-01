# `pico.*` modules

This page describes the modules that wrap the `pico_*` libraries of the
pico-sdk. It only documents the functions that aren't part of the pico-sdk, or
that significantly deviate from the underlying C function. For the others, the
[pico-sdk documentation](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html)
is authoritative.

The test modules can be useful as usage examples.

## `pico`

**Library:** [`pico_base`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_base),
module: [`pico`](../lib/pico.c),
tests: [`pico.test`](../lib/pico.test.lua)

- `board: string`\
  The name of the target board (`PICO_BOARD`).

- `build_type: string`\
  The CMake build type (`Debug`, `Release`, ...).

- `build_target: string`\
  The name of the build target for the program.

- `flash_binary_start: integer`\
  `flash_binary_end, integer`\
  The start and end address of the binary in flash memory.

## `pico.i2c_slave`

**Library:** [`pico_i2c_slave`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_i2c_slave),
module: [`pico.i2c_slave`](../lib/pico.i2c_slave.c),
tests: [`pico.i2c_slave.test`](../lib/pico.i2c_slave.test.lua)

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
module: [`pico.multicore`](../lib/pico.multicore.c),
tests: [`pico.multicore.test`](../lib/pico.multicore.test.lua)

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
module: [`pico.multicore.fifo`](../lib/pico.multicore.fifo.c),
tests: [`pico.multicore.fifo.test`](../lib/pico.multicore.fifo.test.lua)

- `ROE: integer = hardware.regs.FIFO_ST_ROE_BITS`\
  `WOF: integer = hardware.regs.FIFO_ST_WOF_BITS`\
  `RDY: integer = hardware.regs.FIFO_ST_RDY_BITS`\
  `VLD: integer = hardware.regs.FIFO_ST_VLD_BITS`\
  Useful constants for `get_status()`.

- `push_blocking(data)`\
  `push_timeout_us(data, timeout) -> boolean`\
  These functions yield if the FIFO is full and threading is enabled. They are
  implemented as busy loops, because there is no IRQ for the `RDY` flag.

- `pop_blocking() -> integer`\
  `pop_timeout_us(timeout) -> integer | nil`\
  These functions yield if the FIFO is empty and the IRQ handler is enabled.

- `enable_irq(enable) -> Thread`\
  [Enable or disable](common.md#irq-enablers) the SIO IRQ handler
  (`SIO_IRQ_PROCx`).

## `pico.platform`

**Library:** [`pico_platform`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_platform),
module: [`pico.platform`](../lib/pico.platform.c),
tests: [`pico.platform.test`](../lib/pico.platform.test.lua)

TODO

## `pico.stdio`

**Library:** [`pico_stdio`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio),
module: [`pico.stdio`](../lib/pico.stdio.c),
tests: [`pico.stdio.test`](../lib/pico.stdio.test.lua)

TODO

### `pico.stdio.semihosting`

**Library:** [`pico_stdio_semihosting`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_semihosting),
module: [`pico.stdio.semihosting`](../lib/pico.stdio.semihosting.c)

TODO

### `pico.stdio.uart`

**Library:** [`pico_stdio_uart`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_uart),
module: [`pico.stdio.uart`](../lib/pico.stdio.uart.c),
tests: [`pico.stdio.uart.test`](../lib/pico.stdio.uart.test.lua)

TODO

### `pico.stdio.usb`

**Library:** [`pico_stdio_usb`](https://www.raspberrypi.com/documentation/pico-sdk/runtime.html#pico_stdio_usb),
module: [`pico.stdio.usb`](../lib/pico.stdio.usb.c)

TODO

## `pico.stdlib`

**Library:** [`pico_stdlib`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_stdlib),
module: [`pico.stdlib`](../lib/pico.stdlib.c),
tests: [`pico.stdlib.test`](../lib/pico.stdlib.test.lua)

TODO

## `pico.time`

**Library:** [`pico_time`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_time),
module: [`pico.time`](../lib/pico.time.c),
tests: [`pico.time.test`](../lib/pico.time.test.lua)

TODO

## `pico.unique_id`

**Library:** [`pico_unique_id`](https://www.raspberrypi.com/documentation/pico-sdk/high_level.html#pico_unique_id),
module: [`pico.unique_id`](../lib/pico.unique_id.c),
tests: [`pico.unique_id.test`](../lib/pico.unique_id.test.lua)

TODO
