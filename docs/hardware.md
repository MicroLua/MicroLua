# `hardware.*` modules

<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

This page describes the modules that wrap the `hardware_*` libraries of the
Pico SDK. It only documents the functions that aren't part of the Pico SDK, or
that significantly deviate from the underlying C function. For the others, the
[Pico SDK documentation](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html)
is authoritative.

The test modules can be useful as usage examples.

## `hardware.adc`

**Library:** [`hardware_adc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_adc),
header: [`hardware/adc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_adc/include/hardware/adc.h),
sources: [`hardware_adc`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_adc)\
**Module:** [`hardware.adc`](../lib/hardware.adc.c),
build target: `mlua_mod_hardware_adc`,
tests: [`hardware.adc.test`](../lib/hardware.adc.test.lua)

- `fifo_get_blocking() -> integer` *[yields]*\
  Wait for data in the ADC FIFO, then pop and return a value. Yields if the FIFO
  is empty and the IRQ handler is enabled.

- `fifo_enable_irq(enable)`\
  [Enable or disable](core.md#irq-enablers) the ADC FIFO IRQ handler
  (`ADC_IRQ_FIFO`).

## `hardware.base`

**Library:** [`hardware_base`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_base),
header: [`hardware/address_mapped.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_base/include/hardware/address_mapped.h),
sources: [`hardware_base`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_base)\
**Module:** [`hardware.base`](../lib/hardware.base.c),
build target: `mlua_mod_hardware_base`,
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
header: [`hardware/clocks.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_clocks/include/hardware/clocks.h),
sources: [`hardware_clocks`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_clocks)\
**Module:** [`hardware.clocks`](../lib/hardware.clocks.c),
build target: `mlua_mod_hardware_clocks`,
tests: [`hardware.clocks.test`](../lib/hardware.clocks.test.lua)

> [!NOTE]
> Resus functionality isn't implemented yet.

## `hardware.flash`

**Library:** [`hardware_flash`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_flash),
header: [`hardware/flash.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash/include/hardware/flash.h),
sources: [`hardware_flash`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash)\
**Module:** [`hardware.flash`](../lib/hardware.flash.c),
build target: `mlua_mod_hardware_flash`,
tests: [`hardware.flash.test`](../lib/hardware.flash.test.lua)

## `hardware.gpio`

**Library:** [`hardware_gpio`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_gpio),
header: [`hardware/gpio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio/include/hardware/gpio.h),
sources: [`hardware_gpio`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio)\
**Module:** [`hardware.gpio`](../lib/hardware.gpio.c),
build target: `mlua_mod_hardware_gpio`,
tests: [`hardware.gpio.test`](../lib/hardware.gpio.test.lua)

> [!NOTE]
> Raw IRQ handler functionality isn't implemented yet.

- `get_pad(gpio) -> boolean`\
  Read the raw value of the pin, bypassing any muxing or overrides. The
  corresponding C function `gpio_get_pad()` isn't defined in `hardware/gpio.h`
  and isn't documented.

- `set_irq_callback(callback) -> Thread`\
  Set the generic callback used for GPIO IRQ events, or remove the callback if
  `callback` is `nil`. Returns the [event handler thread](core.md#callbacks).

  - `callback(gpio, event_mask)`\
    The callback to be called on GPIO IRQ events.

  The callback can be removed either by killing the returned thread, or by
  calling `set_irq_callback()` with a `nil` callback.

- `set_irq_enabled_with_callback(gpio, event_mask, enabled, callback) -> Thread`\
  Update the event mask for a GPIO, set the generic callback used for GPIO IRQ
  events, and enable GPIO IRQs (`IO_IRQ_BANK0`). Returns the
  [event handler thread](core.md#callbacks).

## `hardware.i2c`

**Library:** [`hardware_i2c`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_i2c),
header: [`hardware/i2c.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_i2c/include/hardware/i2c.h),
sources: [`hardware_i2c`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_i2c)\
**Module:** [`hardware.i2c`](../lib/hardware.i2c.c),
build target: `mlua_mod_hardware_i2c`,
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

- `I2C:write_blocking_until(addr, src, nostop, until) -> integer | nil` *[yields]*\
  `I2C:write_timeout_us(addr, src, nostop, timeout_us) -> integer | nil` *[yields]*\
  `I2C:write_blocking(addr, src, nostop) -> integer | nil` *[yields]*\
  If the write fails, returns `fail` and an error code. Yields until the write
  completes if the IRQ handler is enabled.

- `I2C:read_blocking_until(addr, len, nostop, until) -> string | nil` *[yields]*\
  `I2C:read_timeout_us(addr, len, nostop, until) -> string | nil` *[yields]*\
  `I2C:read_blocking(addr, len, nostop) -> string | nil` *[yields]*\
  If the read fails, returns `fail` and an error code. Yields until the read completes if the IRQ handler is enabled.

- `I2C:read_data_cmd() -> integer`\
  Read the `IC_DATA_CMD` register. This is identical to `I2C:read_byte_raw()`,
  but also returns the `FIRST_DATA_BYTE` flag.

- `I2C:enable_irq(enable)`\
  [Enable or disable](core.md#irq-enablers) the I2C IRQ handler (`I2Cx_IRQ`).

## `hardware.irq`

**Library:** [`hardware_irq`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_irq),
header: [`hardware/irq.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_irq/include/hardware/irq.h),
sources: [`hardware_irq`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_irq)\
**Module:** [`hardware.irq`](../lib/hardware.irq.c),
build target: `mlua_mod_hardware_irq`,
tests: [`hardware.irq.test`](../lib/hardware.irq.test.lua)

> [!NOTE]
> IRQ handlers can only be set for user IRQs (`FIRST_USER_IRQ` ..
> `FIRST_USER_IRQ` + `NUM_USER_IRQS`).

- `set_handler(num, handler, priority = nil) -> Thread`\
  Set a handler for the user IRQ `num`. When `priority` is missing, `nil` or
  negative, an exclusive IRQ handler is set. Otherwise, a shared handler with
  the given priority is set. Returns the
  [event handler thread](core.md#callbacks).

  - `handler(num)`\
    The handler to be called when the user IRQ is triggered.

- `set_exclusive_handler(num, handler) -> Thread`\
  Set an exclusive handler for the user IRQ `num`. Returns the
  [event handler thread](core.md#callbacks).

  - `handler(num)`\
    The handler to be called when the user IRQ is triggered.

- `add_shared_handler(num, handler, priority) -> Thread`\
  Add a shared handler for the user IRQ `num`. Returns the
  [event handler thread](core.md#callbacks).

  - `handler(num)`\
    The handler to be called when the user IRQ is triggered.

- `remove_handler(num)`\
  Remove the IRQ handler for the user IRQ `num`. Alternatively, the thread
  returned when setting the handler can be killed.

## `hardware.pll`

**Library:** [`hardware_pll`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pll),
header: [`hardware/pll.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pll/include/hardware/pll.h),
sources: [`hardware_pll`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pll)\
**Module:** [`hardware.pll`](../lib/hardware.pll.c),
build target: `mlua_mod_hardware_pll`,
tests: [`hardware.pll.test`](../lib/hardware.pll.test.lua)

## `hardware.regs.*`

**Library:** `hardware_regs`,
sources: [`hardware_regs`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs)\
**Modules:** `hardware.regs.*`,
build targets: `mlua_mod_hardware_regs_*`,
tests: [`hardware.regs.test`](../lib/hardware.regs.test.lua)

The `hardware.regs.*` modules expose constants defined in the
[`hardware/regs/*.h`](https://github.com/raspberrypi/pico-sdk/tree/master/src/rp2040/hardware_regs/include/hardware/regs)
headers. Each module is auto-generated from the correponding header. Thanks
to [read-only tables](core.md#read-only-tables), they use very little RAM and
a reasonable amount of flash, despite some of the headers being huge.

The constants have the prefix corresponding to the header file name stripped.
For example, all constants in `uart.h` have a `UART_` prefix, and
`UART_UARTDR_PE_BITS` becomes `UARTDR_PE_BITS`. The following constants are
excluded:

- String constants whose name ends with `_ACCESS` and whose value has a length
  of 2 or 3. These describe register access (read, write, etc.) and aren't
  useful at runtime.
- String constants whose name ends with `_RESET` and whose value is `"-"`. These
  are register fields whose reset value isn't known.
- Constants whose name starts with `isr_`. These are aliases for IRQ handler
  slots defined in `intctrl.h`, which aren't useful in Lua.

Here's the list of modules and their source header for reference:

- `hardware.regs.adc`: [`hardware/regs/adc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/adc.h)
- `hardware.regs.addressmap`: [`hardware/regs/addressmap.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/addressmap.h)
- `hardware.regs.busctrl`: [`hardware/regs/busctrl.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/busctrl.h)
- `hardware.regs.clocks`: [`hardware/regs/clocks.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/clocks.h)
- `hardware.regs.dma`: [`hardware/regs/dma.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/dma.h)
- `hardware.regs.dreq`: [`hardware/regs/dreq.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/dreq.h)
- `hardware.regs.i2c`: [`hardware/regs/i2c.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/i2c.h)
- `hardware.regs.intctrl`: [`hardware/regs/intctrl.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/intctrl.h)
- `hardware.regs.io_bank0`: [`hardware/regs/io_bank0.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/io_bank0.h)
- `hardware.regs.io_qspi`: [`hardware/regs/io_qspi.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/io_qspi.h)
- `hardware.regs.m0plus`: [`hardware/regs/m0plus.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/m0plus.h)
- `hardware.regs.pads_bank0`: [`hardware/regs/pads_bank0.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/pads_bank0.h)
- `hardware.regs.pads_qspi`: [`hardware/regs/pads_qspi.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/pads_qspi.h)
- `hardware.regs.pio`: [`hardware/regs/pio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/pio.h)
- `hardware.regs.pll`: [`hardware/regs/pll.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/pll.h)
- `hardware.regs.psm`: [`hardware/regs/psm.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/psm.h)
- `hardware.regs.pwm`: [`hardware/regs/pwm.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/pwm.h)
- `hardware.regs.resets`: [`hardware/regs/resets.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/resets.h)
- `hardware.regs.rosc`: [`hardware/regs/rosc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/rosc.h)
- `hardware.regs.rtc`: [`hardware/regs/rtc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/rtc.h)
- `hardware.regs.sio`: [`hardware/regs/sio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/sio.h)
- `hardware.regs.spi`: [`hardware/regs/spi.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/spi.h)
- `hardware.regs.ssi`: [`hardware/regs/ssi.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/ssi.h)
- `hardware.regs.syscfg`: [`hardware/regs/syscfg.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/syscfg.h)
- `hardware.regs.sysinfo`: [`hardware/regs/sysinfo.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/sysinfo.h)
- `hardware.regs.tbman`: [`hardware/regs/tbman.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/tbman.h)
- `hardware.regs.timer`: [`hardware/regs/timer.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/timer.h)
- `hardware.regs.uart`: [`hardware/regs/uart.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/uart.h)
- `hardware.regs.usb`: [`hardware/regs/usb.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/usb.h)
- `hardware.regs.usb_device_dpram`: [`hardware/regs/usb_device_dpram.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/usb_device_dpram.h)
- `hardware.regs.vreg_and_chip_reset`: [`hardware/regs/vreg_and_chip_reset.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/vreg_and_chip_reset.h)
- `hardware.regs.watchdog`: [`hardware/regs/watchdog.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/watchdog.h)
- `hardware.regs.xip`: [`hardware/regs/xip.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/xip.h)
- `hardware.regs.xosc`: [`hardware/regs/xosc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/xosc.h)

## `hardware.resets`

**Library:** [`hardware_resets`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_resets),
header: [`hardware/resets.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_resets/include/hardware/resets.h),
sources: [`hardware_resets`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_resets)\
**Module:** [`hardware.resets`](../lib/hardware.resets.c),
build target: `mlua_mod_hardware_resets`,
tests: [`hardware.resets.test`](../lib/hardware.resets.test.lua)

The reset bitmasks are avaialble in the `hardware.regs.resets` module as
`RESET_*_BITS`.

- `unreset_block_wait(bits)`\
  This function blocks without yielding.

## `hardware.rtc`

**Library:** [`hardware_rtc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_rtc),
header: [`hardware/rtc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_rtc/include/hardware/rtc.h),
sources: [`hardware_rtc`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_rtc)\
**Module:** [`hardware.rtc`](../lib/hardware.rtc.c),
build target: `mlua_mod_hardware_rtc`,
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
  [event handler thread](core.md#callbacks).

  - `callback()`\
    The callback to be called when the RTC time matches `t`.

  The callback can be removed either by killing the returned thread, or by
  calling `set_alarm()` with a `nil` callback.

## `hardware.spi`

**Library:** [`hardware_spi`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_spi),
header: [`hardware/spi.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_spi/include/hardware/spi.h),
sources: [`hardware_spi`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_spi)\
**Module:** [`hardware.spi`](../lib/hardware.spi.c),
build target: `mlua_mod_hardware_spi`,
tests: [`hardware.spi.test`](../lib/hardware.spi.test.lua)

This module defines the `hardware.spi.SPI` class, which exposes the
functionality for one SPI peripheral. All library functions that take an
`spi_inst_t*` as a first argument are exposed as methods on the `SPI` class.
The class is instantiated once for each SPI peripheral of the target, and the
instances can be accessed by indexing the module with the instance index. The
default SPI peripheral, if defined, can be accessed as `default`.

> [!NOTE]
> There are no 16-bit variants of the write / read functions, since the latter
> auto-detect the word size from the peripheral configuration.

- `[0]: SPI`\
  `[1]: SPI`\
  The `SPI` instances.

- `NUM: integer`\
  The number of SPI peripherals available on the target.

- `default: SPI | boolean`\
  The default `SPI` instance, or `false` if `PICO_DEFAULT_SPI` is unset.

- `SPI:regs_base() -> integer`\
  Return the base address of the peripheral registers (`SPIx_BASE`).

- `SPI:write_read_blocking(src) -> string`\
  `SPI:write_blocking(src)`\
  `SPI:read_blocking(tx_data, len) -> string`\
  Write to and / or read from the SPI peripheral. Yields until the write and /
  or read completes if the IRQ handler is enabled. The word size is
  auto-detected from the peripheral configuration. For word sizes >8 bits,
  `src` must provide two bytes per word (little-endian).

- `SPI:enable_loopback(enable)`\
  Enable or disable the loopback mode of the SPI peripheral (`LBM` in `SSPCR1`).

- `SPI:enable_irq(enable)`\
  [Enable or disable](core.md#irq-enablers) the SPI IRQ handler (`SPIx_IRQ`).

## `hardware.sync`

**Library:** [`hardware_sync`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_sync),
header: [`hardware/sync.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_sync/include/hardware/sync.h),
sources: [`hardware_sync`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_sync)\
**Module:** [`hardware.sync`](../lib/hardware.sync.c),
build target: `mlua_mod_hardware_sync`,
tests: [`hardware.sync.test`](../lib/hardware.sync.test.lua)

> [!NOTE]
> Spin lock functionality isn't exposed to Lua, because spin locks are supposed
> to be held for a short duration only, but this can't be achieved from Lua.

## `hardware.timer`

**Library:** [`hardware_timer`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_timer),
header: [`hardware/timer.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_timer/include/hardware/timer.h),
sources: [`hardware_timer`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_timer)\
**Module:** [`hardware.timer`](../lib/hardware.timer.c),
build target: `mlua_mod_hardware_timer`,
tests: [`hardware.timer.test`](../lib/hardware.timer.test.lua)

- `set_callback(alarm_num, callback) -> Thread`\
  Set the callback for the hardware timer `alarm_num`. The callback is called
  with the arguments `(alarm_num)`. Returns the
  [event handler thread](core.md#callbacks).

  - `callback(alarm_num)`\
    The callback to be called when the timer triggers.

  The callback can be removed either by killing the returned thread, or by
  calling `set_callback()` with a `nil` callback.

## `hardware.uart`

**Library:** [`hardware_uart`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_uart),
header: [`hardware/uart.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_uart/include/hardware/uart.h),
sources: [`hardware_uart`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_uart)\
**Module:** [`hardware.uart`](../lib/hardware.uart.c),
build target: `mlua_mod_hardware_uart`,
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

- `UART:tx_wait_blocking()` *[yields]*\
  Wait until the TX FIFO is empty. Yields if the FIFO isn't empty. This function
  is implemented with polling, because there is no IRQ for "TX FIFO empty".

- `UART:write_blocking(src)` *[yields]*\
  Yields until the write completes if the IRQ handler is enabled.

- `UART:read_blocking(len) -> string` *[yields]*\
  `UART:getc() -> integer` *[yields]*\
  Yields until the read completes if the IRQ handler is enabled.

- `UART:putc_raw(c)`\
  `UART:putc(c)`\
  `UART:puts(c)`\
  These functions block without yielding if the TX FIFO is full.

- `UART:is_readable_within_us(us) -> boolean` *[yields]*\
  Wait until the RX FIFO is non-empty, or until the timeout elapses. Yields if
  the FIFO is empty and the IRQ handler is enabled.

- `UART:enable_loopback(enable)`\
  Enable or disable the loopback mode of the UART (`LBE` in `UARTCR`).

- `UART:enable_irq(enable)`\
  [Enable or disable](core.md#irq-enablers) the UART IRQ handler (`UARTx_IRQ`).

## `hardware.vreg`

**Library:** [`hardware_vreg`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_vreg),
header: [`hardware/vreg.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_vreg/include/hardware/vreg.h),
sources: [`hardware_vreg`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_vreg)\
**Module:** [`hardware.vreg`](../lib/hardware.vreg.c),
build target: `mlua_mod_hardware_vreg`,
tests: [`hardware.vreg.test`](../lib/hardware.vreg.test.lua)

## `hardware.watchdog`

**Library:** [`hardware_watchdog`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_watchdog),
header: [`hardware/watchdog.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_watchdog/include/hardware/watchdog.h),
sources: [`hardware_watchdog`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_watchdog)\
**Module:** [`hardware.watchdog`](../lib/hardware.watchdog.c),
build target: `mlua_mod_hardware_watchdog`,
tests: [`hardware.watchdog.test`](../lib/hardware.watchdog.test.lua)

## `hardware.xosc`

**Library:** [`hardware_xosc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_xosc),
header: [`hardware/xosc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_xosc/include/hardware/xosc.h),
sources: [`hardware_xosc`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_xosc)\
**Module:** [`hardware.xosc`](../lib/hardware.xosc.c),
build target: `mlua_mod_hardware_xosc`,
tests: [`hardware.xosc.test`](../lib/hardware.xosc.test.lua)
