# `hardware.*` modules

This page describes the modules that wrap the `hardware_*` libraries of the
pico-sdk. It only documents the functions that aren't part of the pico-sdk, or
that significantly deviate from the underlying C function. For the others, the
[pico-sdk documentation](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html)
is authoritative.

The test modules can be useful as usage examples.

## `hardware.adc`

**Library:** [`hardware_adc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_adc),
module: [`hardware.adc`](../lib/hardware.adc.c),
tests: [`hardware.adc.test`](../lib/hardware.adc.test.lua)

- `fifo_get_blocking() -> integer`\
  Wait for data in the ADC FIFO, then pop and return a value. Yields if the FIFO
  is empty and the IRQ handler is enabled.

- `fifo_enable_irq(enable)`\
  [Enable or disable](common.md#irq-enablers) the ADC FIFO IRQ handler
  (`ADC_IRQ_FIFO`).

## `hardware.base`

**Library:** [`hardware_base`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_base),
module: [`hardware.base`](../lib/hardware.base.c),
tests: [`hardware.base.test`](../lib/hardware.base.test.lua)

The [`hardware.regs.*`](#hardwareregs) modules provide constants for peripheral
block addresses (`hardware.regs.addressmap`), register offsets and bit masks.

- `read8(address, count = 1) -> (value, ...)`\
  Read `count` 8-bit values from consecutive addresses starting at `address`.

- `read16(address, count = 1) -> (value, ...)`\
  Read `count` 16-bit values from consecutive addresses starting at `address`,
  which must be 16-bit aligned.

- `read32(address, count = 1) -> (value, ...)`\
  Read `count` 32-bit values from consecutive addresses starting at `address`,
  which must be 32-bit aligned.

- `write8(address, [value, ...])`\
  Write 8-bit values to consecutive addresses starting at `address`.

- `write16(address, [value, ...])`\
  Write 16-bit values to consecutive addresses starting at `address`, which must
  be 16-bit aligned.

- `write32(address, [value, ...])`\
  Write 32-bit values to consecutive addresses starting at `address`, which must
  be 32-bit aligned.

## `hardware.clocks`

**Library:** [`hardware_clocks`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_clocks),
module: [`hardware.clocks`](../lib/hardware.clocks.c),
tests: [`hardware.clocks.test`](../lib/hardware.clocks.test.lua)

> [!NOTE]
> Resus functionality isn't implemented yet.

## `hardware.flash`

**Library:** [`hardware_flash`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_flash),
module: [`hardware.flash`](../lib/hardware.flash.c),
tests: [`hardware.flash.test`](../lib/hardware.flash.test.lua)

## `hardware.gpio`

**Library:** [`hardware_gpio`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_gpio),
module: [`hardware.gpio`](../lib/hardware.gpio.c),
tests: [`hardware.gpio.test`](../lib/hardware.gpio.test.lua)

> [!NOTE]
> Raw IRQ handler functionality isn't implemented yet.

- `get_pad(gpio) -> boolean`\
  Read the raw value of the pin, bypassing any muxing or overrides. The
  corresponding C function `gpio_get_pad()` isn't defined in `hardware/gpio.h`
  and isn't documented.

- `set_irq_callback(callback) -> Thread`\
  Set the generic callback used for GPIO IRQ events, or remove the callback if
  `callback` is `nil`. Returns the [event handler thread](common.md#callbacks).

  - `callback(gpio, event_mask)`\
    The callback to be called on GPIO IRQ events.

  The callback can be removed either by killing the returned thread, or by
  calling `set_irq_callback()` with a `nil` callback.

- `set_irq_enabled_with_callback(gpio, event_mask, enabled, callback) -> Thread`\
  Update the event mask for a GPIO, set the generic callback used for GPIO IRQ
  events, and enable GPIO IRQs (`IO_IRQ_BANK0`). Returns the
  [event handler thread](common.md#callbacks).

## `hardware.i2c`

**Library:** [`hardware_i2c`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_i2c),
module: [`hardware.i2c`](../lib/hardware.i2c.c),
tests: [`hardware.i2c.test`](../lib/hardware.i2c.test.lua)

This module defines the `hardware.i2c.I2C` class, which exposes the
functionality for one I2C peripheral. All library functions that take an
`i2c_inst_t*` as a first argument are exposed as methods on the `I2C` class.
The class is instantiated once for each I2C peripheral of the target, and the
instances can be accessed by indexing the module with the instance index. The
default I2C peripheral, if defined, can be accessed as `default`.

> [!NOTE]
> Blocking writes and reads with per-char timeouts, as well as raw blocking
> writes and reads, aren't supported yet.

- `[0]: I2C`\
  `[1]: I2C`\
  The `I2C` instances.

- `NUM: integer`\
  The number of I2C peripherals available on the target.

- `default: I2C | boolean`\
  The default `I2C` instance, or `false` if `PICO_DEFAULT_I2C` is unset.

- `I2C:regs_base() -> integer`\
  Return the base address of the peripheral registers (`I2Cx_BASE`).

- `I2C:write_blocking_until(addr, src, nostop, until) -> integer`\
  `I2C:write_timeout_us(addr, src, nostop, timeout_us) -> integer`\
  `I2C:write_blocking(addr, src, nostop) -> integer`\
  If the write fails, returns `fail` and an error code. Yields until the write
  completes if the IRQ handler is enabled.

- `I2C:read_blocking_until(addr, len, nostop, until) -> string`\
  `I2C:read_timeout_us(addr, len, nostop, until) -> string`\
  `I2C:read_blocking(addr, len, nostop) -> string`\
  If the read fails, returns `fail` and an error code. Yields until the read completes if the IRQ handler is enabled.

- `I2C:read_data_cmd() -> integer`\
  Read the `IC_DATA_CMD` register. This is identical to `I2C:read_byte_raw()`,
  but also returns the `FIRST_DATA_BYTE` flag.

- `I2C:enable_irq(enable)`\
  [Enable or disable](common.md#irq-enablers) the I2C IRQ handler (`I2Cx_IRQ`).

## `hardware.irq`

**Library:** [`hardware_irq`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_irq),
module: [`hardware.irq`](../lib/hardware.irq.c),
tests: [`hardware.irq.test`](../lib/hardware.irq.test.lua)

> [!NOTE]
> IRQ handlers can only be set for user IRQs (`FIRST_USER_IRQ` ..
> `FIRST_USER_IRQ` + `NUM_USER_IRQS`).

- `set_handler(num, handler, priority = nil) -> Thread`\
  Set a handler for the user IRQ `num`. When `priority` is missing, `nil` or
  negative, an exclusive IRQ handler is set. Otherwise, a shared handler with
  the given priority is set. Returns the
  [event handler thread](common.md#callbacks).

  - `handler(num)`\
    The handler to be called when the user IRQ is triggered.

- `set_exclusive_handler(num, handler) -> Thread`\
  Set an exclusive handler for the user IRQ `num`. Returns the
  [event handler thread](common.md#callbacks).

  - `handler(num)`\
    The handler to be called when the user IRQ is triggered.

- `add_shared_handler(num, handler, priority) -> Thread`\
  Add a shared handler for the user IRQ `num`. Returns the
  [event handler thread](common.md#callbacks).

  - `handler(num)`\
    The handler to be called when the user IRQ is triggered.

- `remove_handler(num)`\
  Remove the IRQ handler for the user IRQ `num`. Alternatively, the thread
  returned when setting the handler can be killed.

## `hardware.pll`

**Library:** [`hardware_pll`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pll),
module: [`hardware.pll`](../lib/hardware.pll.c),
tests: [`hardware.pll.test`](../lib/hardware.pll.test.lua)

## `hardware.regs.*`

## `hardware.resets`

**Library:** [`hardware_resets`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_resets),
module: [`hardware.resets`](../lib/hardware.resets.c),
tests: [`hardware.resets.test`](../lib/hardware.resets.test.lua)

The reset bitmasks are avaialble in the `hardware.regs.resets` module as
`RESET_*_BITS`.

- `unreset_block_wait(bits)`\
  This function blocks without yielding.

## `hardware.rtc`

**Library:** [`hardware_rtc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_rtc),
module: [`hardware.rtc`](../lib/hardware.rtc.c),
tests: [`hardware.rtc.test`](../lib/hardware.rtc.test.lua)

- `set_datetime(t) -> boolean`\
  Set the RTC to the specified time. `t` is a table containing the fields of
  [`datetime_t`](https://www.raspberrypi.com/documentation/pico-sdk/structdatetime__t.html).

- `get_datetime() -> table`\
  Get the current time from the RTC. Returns a table containing the fields of
  [`datetime_t`](https://www.raspberrypi.com/documentation/pico-sdk/structdatetime__t.html).

- `set_alarm(t, callback) -> Thread`\
  Set a callback to be called at specific times in the future. `t` is a table
  containing a subset of the fields of
  [`datetime_t`](https://www.raspberrypi.com/documentation/pico-sdk/structdatetime__t.html).
  Unset fields will not be matched on. Returns the
  [event handler thread](common.md#callbacks).

  - `callback()`\
    The callback to be called when the RTC time matches `t`.

  The callback can be removed either by killing the returned thread, or by
  calling `set_alarm()` with a `nil` callback.

## `hardware.sync`

**Library:** [`hardware_sync`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_sync),
module: [`hardware.sync`](../lib/hardware.sync.c),
tests: [`hardware.sync.test`](../lib/hardware.sync.test.lua)

> [!NOTE]
> Spin lock functionality isn't exposed to Lua, because spin locks are supposed
> to be held for a short duration only, but this can't be achieved from Lua.

## `hardware.timer`

**Library:** [`hardware_timer`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_timer),
module: [`hardware.timer`](../lib/hardware.timer.c),
tests: [`hardware.timer.test`](../lib/hardware.timer.test.lua)

- `set_callback(alarm_num, callback) -> Thread`\
  Set the callback for the hardware timer `alarm_num`. The callback is called
  with the arguments `(alarm_num)`. Returns the
  [event handler thread](common.md#callbacks).

  - `callback(alarm_num)`\
    The callback to be called when the timer triggers.

  The callback can be removed either by killing the returned thread, or by
  calling `set_callback()` with a `nil` callback.

## `hardware.uart`

**Library:** [`hardware_uart`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_uart),
module: [`hardware.uart`](../lib/hardware.uart.c),
tests: [`hardware.uart.test`](../lib/hardware.uart.test.lua)

This module defines the `hardware.uart.UART` class, which exposes the
functionality for one UART peripheral. All library functions that take an
`uart_inst_t*` as a first argument are exposed as methods on the `UART` class.
The class is instantiated once for each UART peripheral of the target, and the
instances can be accessed by indexing the module with the instance index. The
default UART peripheral, if defined, can be accessed as `default`.

- `[0]: UART`\
  `[1]: UART`\
  The `UART` instances.

- `NUM: integer`\
  The number of UART peripherals available on the target.

- `default: UART | boolean`\
  The default `UART` instance, or `false` if `PICO_DEFAULT_UART` is unset.

- `UART:regs_base() -> integer`\
  Return the base address of the peripheral registers (`UARTx_BASE`).

- `UART:is_tx_busy() -> boolean`\
  Return `true` iff the TX FIFO is busy.

- `UART:tx_wait_blocking()`\
  Wait until the TX FIFO is empty. Yields if the FIFO isn't empty. This function
  is implemented with polling, because there is no IRQ for "TX FIFO empty".

- `UART:write_blocking(src)`\
  Yields until the write completes if the IRQ handler is enabled.

- `UART:read_blocking(len) -> string`\
  `UART:getc() -> integer`\
  Yields until the read completes if the IRQ handler is enabled.

- `UART:putc_raw(c)`\
  `UART:putc(c)`\
  `UART:puts(c)`\
  These functions block without yielding if the TX FIFO is full. This will be
  fixed eventually.

- `UART:is_readable_within_us(us) -> boolean`\
  Wait until the RX FIFO is non-empty, or until the timeout elapses. Yields if
  the FIFO is empty and the IRQ handler is enabled.

- `UART:enable_loopback(enable)`\
  Enable or disable the loopback mode of the UART (`LBE` in `UARTCR`).

- `UART:enable_irq(enable)`\
  [Enable or disable](common.md#irq-enablers) the UART IRQ handler
  (`UARTx_IRQ`).

## `hardware.vreg`

**Library:** [`hardware_vreg`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_vreg),
module: [`hardware.vreg`](../lib/hardware.vreg.c),
tests: [`hardware.vreg.test`](../lib/hardware.vreg.test.lua)

## `hardware.watchdog`

**Library:** [`hardware_watchdog`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_watchdog),
module: [`hardware.watchdog`](../lib/hardware.watchdog.c),
tests: [`hardware.watchdog.test`](../lib/hardware.watchdog.test.lua)

## `hardware.xosc`

**Library:** [`hardware_xosc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_xosc),
module: [`hardware.xosc`](../lib/hardware.xosc.c),
tests: [`hardware.xosc.test`](../lib/hardware.xosc.test.lua)
