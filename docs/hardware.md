<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

# `hardware.*` modules

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
**Module:** [`hardware.adc`](../lib/pico/hardware.adc.c),
build target: `mlua_mod_hardware.adc`,
tests: [`hardware.adc.test`](../lib/pico/hardware.adc.test.lua)

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
**Module:** [`hardware.base`](../lib/pico/hardware.base.c),
build target: `mlua_mod_hardware.base`,
tests: [`hardware.base.test`](../lib/pico/hardware.base.test.lua)

The [`hardware.regs.*`](#hardwareregs) modules provide constants for peripheral
block pointers (`hardware.regs.addressmap`), register offsets and bit masks.

- `read8(ptr, count = 1) -> (value, ...)`\
  Read `count` 8-bit values from consecutive addresses starting at `ptr`.

- `read16(ptr, count = 1) -> (value, ...)`\
  Read `count` 16-bit values from consecutive addresses starting at `ptr`, which
  must be 16-bit aligned.

- `read32(ptr, count = 1) -> (value, ...)`\
  Read `count` 32-bit values from consecutive addresses starting at `ptr`, which
  must be 32-bit aligned.

- `write8(ptr, [value, ...])`\
  Write 8-bit values to consecutive addresses starting at `ptr`.

- `write16(ptr, [value, ...])`\
  Write 16-bit values to consecutive addresses starting at `ptr`, which must be
  16-bit aligned.

- `write32(ptr, [value, ...])`\
  Write 32-bit values to consecutive addresses starting at `ptr`, which must be
  32-bit aligned.

## `hardware.clocks`

**Library:** [`hardware_clocks`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_clocks),
header: [`hardware/clocks.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_clocks/include/hardware/clocks.h),
sources: [`hardware_clocks`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_clocks)\
**Module:** [`hardware.clocks`](../lib/pico/hardware.clocks.c),
build target: `mlua_mod_hardware.clocks`,
tests: [`hardware.clocks.test`](../lib/pico/hardware.clocks.test.lua)

> [!NOTE]
> Resus functionality isn't implemented yet.

- `check_sys_clock_khz(freq_khz) -> (integer, integer, integer) | nil`

## `hardware.dma`

**Library:** [`hardware_dma`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_dma),
header: [`hardware/dma.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_dma/include/hardware/dma.h),
sources: [`hardware_dma`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_dma)\
**Module:** [`hardware.dma`](../lib/pico/hardware.dma.c),
build target: `mlua_mod_hardware.dma`,
tests: [`hardware.dma.test`](../lib/pico/hardware.dma.test.lua)

This module exposes DMA functionality.

- `NUM_CHANNELS: integer`\
  `NUM_TIMERS: integer`\
  The number of DMA channels and timers, respectively.

- `DREQ_TIMERx: integer`\
  `DREQ_FORCE: integer`\
  Transfer request signals for the DMA timers, and for unthrottled transfers.

- `SIZE_8: integer`\
  `SIZE_16: integer`\
  `SIZE_32: integer`\
  DMA transfer sizes.

- `SNIFF_*: integer`\
  Constants related to DMA sniffing.

- `channel_get_default_config(channel) -> Config`\
  Get the default configuration for a channel.

- `get_channel_config(channel) -> Config`\
  Get the current configuration of a channel.

- `regs([channel]) -> pointer`\
  When `channel` is absent or `nil`, return a pointer to the base of the
  peripheral registers (`DMA_BASE`). Otherwise, return a pointer to the base
  of the registers for the given channel.

- `channel_claim(channel)`\
  Mark a channel as used.

- `claim_mask(mask)`\
  Mark multiple channels as used.

- `channel_unclaim(channel)`\
  Mark a channel as no longer used.

- `unclaim_mask(mask)`\
  Mark multiple channels as no longer used.

- `claim_unused_channel(required = true) -> integer`\
  Claim a free channel.

- `channel_is_claimed(channel) -> boolean`\
  Return `true` iff the given channel is claimed.

- `channel_set_config(channel, config, trigger)`\
  Set the configuration for a channel.

- `channel_set_read_addr(channel, ptr, trigger)`\
  Set the initial read address for a channel. `ptr` can be `nil`, a string or a
  [buffer](core.md#buffer-protocol).

- `channel_set_write_addr(channel, ptr, trigger)`\
  Set the initial write address for a channel. `ptr` can be `nil` or a
  [buffer](core.md#buffer-protocol).

- `channel_set_trans_count(channel, count, trigger)`\
  Set the number of bus transfers the channel will do.

- `channel_configure(channel, config, write_ptr, read_ptr, count, trigger)`\
  Configure all parameters of a channel.

- `channel_transfer_from_buffer_now(channel, read_ptr, count)`\
  Start a transfer from a buffer immediately.

- `channel_transfer_to_buffer_now(channel, write_ptr, count)`\
  Start a transfer to a buffer immediately.

- `start_channel_mask(mask)`\
  Start multiple channels simultaneously.

- `channel_start(channel)`\
  Start a channel.

- `channel_abort(channel)`\
  Stop a transfer.

- `channel_get_irq_status(channel) -> boolean`\
  Determine if a channel is asserting its IRQ flag.

- `channel_acknowledge_irq(channel)`\
  Acknowledge and reset the IRQ flag of a channel.

- `channel_is_busy(channel) -> boolean`\
  Return `true` iff the given channel is busy.

- `channel_wait_for_finish_blocking(channel)`\
  Wait for a channel transfer to finish. This function blocks without yielding;
  for non-blocking behavior, use `wait_irq()`.

- `sniffer_enable(channel, mode, force_channel_enable)`\
  Enable sniffing, targeting the given channel.

- `sniffer_set_byte_swap_enabled(enable)`\
  Enable or disable byte swapping for sniffing.

- `sniffer_set_output_invert_enabled(enable)`\
  Enable or disable output inversion for sniffing.

- `sniffer_set_output_reverse_enabled(enable)`\
  Enable or disable output bit reversal for sniffing.

- `sniffer_enable()`\
  Disable sniffing.

- `sniffer_set_data_accumulator(value)`\
  Set the value of the sniffer's data accumulator.

- `sniffer_get_data_accumulator() -> integer`\
  Get the value of the sniffer's data accumulator.

- `timer_claim(timer)`\
  Mark a timer as used.

- `timer_unclaim(timer)`\
  Mark a timer as no longer used.

- `claim_unused_timer(required = true) -> integer`\
  Claim a free timer.

- `timer_is_claimed(timer) -> boolean`\
  Return `true` iff the given timer is claimed.

- `timer_set_fraction(timer, numerator, denominator)`\
  Set the divider for a timer.

- `get_timer_dreq(timer) -> integer`\
  Return the `DREQ` number for the given timer.

- `channel_cleanup(channel)`\
  Clean up a channel after use.

- `wait_irq(mask) -> integer` *[yields]*\
  Wait for one of the channel IRQ flags indicated by the mask to be set. Returns
  the IRQs that are currently pending, masked by `mask`.

- `enable_irq(mask, enable)`\
  [Enable or disable](core.md#irq-enablers) the DMA IRQ handler (`DMA_IRQ_n`)
  for a set of channels. The module uses IRQ 0 on core 0 and IRQ 1 on core 1.

### `Config`

The `Config` type (`hardware.dma.Config`) contains a DMA channel configuration
and encapsulates a [`dma_channel_config`](https://www.raspberrypi.com/documentation/pico-sdk/structdma__channel__config.html)
structure. C library functions that take a `dma_channel_config*` as their first
argument are exposed as methods on the `Config` class. Setters return the
`Config` so that they can be chained.

- `Config:ctrl() -> integer`\
  Return the corresponding field of the
  [`dma_channel_config`](https://www.raspberrypi.com/documentation/pico-sdk/structdma__channel__config.html) structure.

- `Config:set_read_increment(enable) -> Config`\
  Enable or disable the read increment.

- `Config:set_write_increment(enable) -> Config`\
  Enable or disable the write increment.

- `Config:set_dreq(dreq) -> Config`\
  Set the transfer request signal to use.

- `Config:set_chain_to(channel) -> Config`\
  Set the channel to which to chain transfers. Chaining can be disabled by
  chaining a channel to itself.

- `Config:set_transfer_data_size(size) -> Config`\
  Set the size of each data transfer. `size` is one of the `SIZE_*` constants.

- `Config:set_ring(write, size_bits) -> Config`\
  Set address wrapping parameters.

- `Config:set_bswap(enable) -> Config`\
  Enable or disable byte swapping.

- `Config:set_irq_quiet(enable) -> Config`\
  Enable or disable IRQ quiet mode.

- `Config:set_high_priority(enable) -> Config`\
  Enable or disable high-priority treatment.

- `Config:set_enable(enable) -> Config`\
  Enable or disable the channel.

- `Config:set_sniff_enable(enable) -> Config`\
  Enable or disable access by the sniff hardware.

## `hardware.flash`

**Library:** [`hardware_flash`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_flash),
header: [`hardware/flash.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash/include/hardware/flash.h),
sources: [`hardware_flash`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_flash)\
**Module:** [`hardware.flash`](../lib/pico/hardware.flash.c),
build target: `mlua_mod_hardware.flash`,
tests: [`hardware.flash.test`](../lib/pico/hardware.flash.test.lua)

## `hardware.gpio`

**Library:** [`hardware_gpio`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_gpio),
header: [`hardware/gpio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio/include/hardware/gpio.h),
sources: [`hardware_gpio`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_gpio)\
**Module:** [`hardware.gpio`](../lib/pico/hardware.gpio.c),
build target: `mlua_mod_hardware.gpio`,
tests: [`hardware.gpio.test`](../lib/pico/hardware.gpio.test.lua)

> [!NOTE]
> Raw IRQ handler functionality isn't implemented yet.

- `get_pad(gpio) -> boolean`\
  Read the raw value of the pin, bypassing any muxing or overrides. The
  corresponding C function `gpio_get_pad()` isn't defined in `hardware/gpio.h`
  and isn't documented.

- `set_irq_callback(callback) -> Thread`\
  Set the generic callback used for GPIO IRQ events. Returns the
  [event handler thread](core.md#callbacks).

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
**Module:** [`hardware.i2c`](../lib/pico/hardware.i2c.c),
build target: `mlua_mod_hardware.i2c`,
tests: [`hardware.i2c.test`](../lib/pico/hardware.i2c.test.lua)

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

- `I2C:regs() -> pointer`\
  Return a pointer to the base of the peripheral registers (`I2Cx_BASE`).

- `I2C:write_blocking(addr, src, nostop) -> integer | nil` *[yields]*\
  `I2C:write_blocking_until(addr, src, nostop, time) -> integer | nil` *[yields]*\
  `I2C:write_timeout_us(addr, src, nostop, timeout) -> integer | nil` *[yields]*\
  If the write fails, returns `fail` and an error code. Yields until the write
  completes if the IRQ handler is enabled. `time` is an
  [absolute time](mlua.md#absolute-time).

- `I2C:read_blocking(addr, len, nostop) -> string | nil` *[yields]*\
  `I2C:read_blocking_until(addr, len, nostop, time) -> string | nil` *[yields]*\
  `I2C:read_timeout_us(addr, len, nostop, timeout) -> string | nil` *[yields]*\
  If the read fails, returns `fail` and an error code. Yields until the read
  completes if the IRQ handler is enabled. `time` is an
  [absolute time](mlua.md#absolute-time).

- `I2C:read_data_cmd() -> integer`\
  Read the `IC_DATA_CMD` register. This is identical to `I2C:read_byte_raw()`,
  but also returns the `FIRST_DATA_BYTE` flag.

- `I2C:enable_irq(enable)`\
  [Enable or disable](core.md#irq-enablers) the I2C IRQ handler (`I2Cx_IRQ`).

## `hardware.irq`

**Library:** [`hardware_irq`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_irq),
header: [`hardware/irq.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_irq/include/hardware/irq.h),
sources: [`hardware_irq`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_irq)\
**Module:** [`hardware.irq`](../lib/pico/hardware.irq.c),
build target: `mlua_mod_hardware.irq`,
tests: [`hardware.irq.test`](../lib/pico/hardware.irq.test.lua)

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

## `hardware.pio`

**Library:** [`hardware_pio`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pio),
header: [`hardware/pio.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pio/include/hardware/pio.h),
sources: [`hardware_pio`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pio)\
**Module:** [`hardware.pio`](../lib/pico/hardware.pio.c),
build target: `mlua_mod_hardware.pio`,
tests: [`hardware.pio.test`](../lib/pico/hardware.pio.test.lua)

This module exposes PIO functionality. The `PIO` class is instantiated once for
each PIO peripheral of the target, and the instances can be accessed by indexing
the module with the instance index.

- `[0]: PIO`\
  `[1]: PIO`\
  The `PIO` instances.

- `NUM: integer`\
  `NUM_STATE_MACHINES: integer`\
  The number of PIO peripherals available on the target, and the number of state
  machines per PIO peripheral.

- `NUM_IRQS: integer`\
  `NUM_SYS_IRQS: integer`\
  The number of state machine interrupts, and the number of state machine
  interrupts that are routed to the system IRQs.

- `FIFO_JOIN_NONE: integer`\
  `FIFO_JOIN_TX: integer`\
  `FIFO_JOIN_RX: integer`\
  FIFO join states.

- `STATUS_TX_LESSTHAN: integer`\
  `STATUS_RX_LESSTHAN: integer`\
  `mov` status types.

- `pis_interrupt[0-3]: integer`\
  `pis_sm[0-3]_tx_fifo_not_full: integer`\
  `pis_sm[0-3]_rx_fifo_not_empty: integer`\
  Interrupt source numbers.

- `get_default_sm_config() -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gaf2d2a23b08ba74188160469b3fd09936))\
  Get the default state machine configuration.

### PIO programs

A PIO program is a table with the program's instructions at integer indices
and optionally an `origin` key specifying the offset at which the program must
be loaded (as given by the `.origin` directive in PIO assembly).

### `Config`

The `Config` type (`hardware.pio.Config`) contains a state machine configuration
and encapsulates a
[`pio_sm_config`](https://www.raspberrypi.com/documentation/pico-sdk/structpio__sm__config.html)
structure. C library functions that take a `pio_sm_config*` as their first
argument are exposed as methods on the `Config` class. Setters return the
`Config` so that they can be chained.

- `Config:clkdiv() -> integer`\
  `Config:execctrl() -> integer`\
  `Config:shiftctrl() -> integer`\
  `Config:pinctrl() -> integer`\
  Return the corresponding field of the
  [`pio_sm_config`](https://www.raspberrypi.com/documentation/pico-sdk/structpio__sm__config.html) structure.

- `Config:set_out_pins(base, count) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gaf3004bbd996443d0c841664ebc92905c))\
  Set the "out" pins.

- `Config:set_set_pins(base, count) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gada1dff2c00b7d3a1cf722880c8373424))\
  Set the "set" pins.

- `Config:set_in_pins(base) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gac418400e30520ea3961d8977c180a4f9))\
  Set the "in" pins.

- `Config:set_sideset_pins(base) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gad55bf8b410fa1d13bd1bd020587e01d7))\
  Set the "sideset" pins.

- `Config:set_sideset(count, optional, pindirs) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gaf543422206a8dbdc2efea85818dd650e))\
  Set the "sideset" options.

- `Config:set_clkdiv_int_frac(int, frac = 0) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga365abc6d25301810ca5ee11e5b36c763))\
  Set the state machine clock divider from integral and fractional parts (16:8).

- `Config:set_clkdiv(div) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gae8c09c7a4372da95ad777faae51c5a24))\
  Set the state machine clock divider from a floating-point value.

- `Config:set_wrap(target, wrap) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gafb753e8b35bbea9209ca4399a845f89c))\
  Set the wrap addresses.

- `Config:set_jmp_pin(pin) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gaf2bfc66d6427ff92519e38dcb611133b))\
  Set the "jmp" pin.

- `Config:set_in_shift(right, autopush, threshold) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gaed7a6e7dc4f1979c7c62e4773df8c79b))\
  Set up input shifting parameters.

- `Config:set_out_shift(right, autopush, threshold) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga613bed03e10e569f1b7aede74d40a5b5))\
  Set up output shifting parameters.

- `Config:set_fifo_join(join) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gafea1a06362182514518ebd91b2d52fd5))\
  Set up the FIFO joining. `join` is one of the `FIFO_JOIN_*` values.

- `Config:set_out_special(sticky, has_enable_pin, enable_pin) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#ga8a9141ceadf9e735b1e96457865de3f6))\
  Set special output operations.

- `Config:set_mov_status(sel, n) -> Config`
  ([doc](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#gacd24870944ce2f00f3f7847bb3e5d543))\
  Set the source for the `mov status` instruction. `sel` is one of the
  `STATUS_*` values.

### `SM`

The `SM` type (`hardware.pio.SM`) represents a state machine within a PIO
peripheral. C library functions that take a `PIO` and a state machine number as
their first arguments are exposed as methods on the `SM` class.

- `SM:index() -> integer`\
  Return the index of the state machine.

- `SM:set_config(config)`\
  Apply a configuration to the state machine.

- `SM:get_dreq(is_tx) -> integer`\
  Return the `DREQ` number to use for pacing transfers to / from a state
  machine's FIFO.

- `SM:init(pc, config)`\
  Reset the state machine to a consistent state and configure it.

- `SM:set_enabled(enabled)`\
  Enable or disable the state machine.

- `SM:restart()`\
  Restart the state machine with a know state.

- `SM:clkdiv_restart()`\
  Restart the state machine's clock divider from a phase of 0.

- `SM:get_pc() -> integer`\
  Return the current program counter of the state machine.

- `SM:exec(instr)`\
  Immediately execute an instruction on the state machine.

- `SM:is_exec_stalled() -> boolean`\
  Return `true` iff an instruction set by `exec()` is stalled executing.

- `SM:exec_wait_blocking(instr)`\
  Immediately execute an instruction on the state machine and wait for it to
  complete.

- `SM:set_wrap(target, wrap)`\
  Set the wrap configuration of the state machine.

- `SM:set_out_pins(base, count)`\
  Set the "out" pins of the state machine.

- `SM:set_set_pins(base, count)`\
  Set the "set" pins of the state machine.

- `SM:set_in_pins(base)`\
  Set the "in" pins of the state machine.

- `SM:set_sideset_pins(base)`\
  Set the "sideset" pins of the state machine.

- `SM:put(data)`\
  Write a word of data to the state machine's TX FIFO.

- `SM:get() -> integer`\
  Read a word of data from the state machine's RX FIFO.

- `SM:is_rx_fifo_full() -> boolean`\
  `SM:is_rx_fifo_empty() -> boolean`\
  `SM:get_rx_fifo_level() -> integer`\
  `SM:is_tx_fifo_full() -> boolean`\
  `SM:is_tx_fifo_empty() -> boolean`\
  `SM:get_tx_fifo_level() -> integer`\
  Return level information about the state machine's RX and TX FIFOs.

- `SM:put_blocking(data)` *[yields]*\
  Write a word of data to the state machine's TX FIFO, blocking if the FIFO is
  full. Yields if the IRQ handler is enabled for the
  `pis_sm[n]_tx_fifo_not_full` interrupt source.

- `SM:put_blocking(data)` *[yields]*\
  Read a word of data from the state machine's RX FIFO, blocking if the FIFO is
  empty. Yields if the IRQ handler is enabled for the
  `pis_sm[n]_rx_fifo_not_empty` interrupt source.

- `SM:drain_tx_fifo()`\
  Empty out the state machine's TX FIFO.

- `SM:set_clkdiv_int_frac(int, frac = 0)`\
  Set the state machine's clock divider from integral and fractional parts
  (16:8).

- `SM:set_clkdiv(div)`\
  Set the state machine's clock divider from a floating-point value.

- `SM:clear_fifos()`\
  Clear the state machine's TX and RX FIFOs.

- `SM:set_pins(pin_values)`\
  Use the state machine to set a value on all pins for the PIO instance.

- `SM:set_pins_with_mask(pin_values, pin_mask)`\
  Use the state machine to set a value on multiple pins for the PIO instance.

- `SM:set_pindirs_with_mask(pin_dirs, pin_mask)`\
  Use the state machine to set the pin directions for multiple pins for the PIO
  instance.

- `SM:set_consecutive_pindirs(base, count, is_out)`\
  Use the state machine to set the same pin direction for multiple consecutive
  pins for the PIO instance.

- `SM:claim()`\
  Mark the state machine as used.

- `SM:unclaim()`\
  Mark the state machine as no longer used.

- `SM:is_claimed() -> boolean()`\
  Return `true` iff the state machine is claimed.

### `PIO`

The `PIO` type (`hardware.pio.PIO`) represents a PIO peripheral. C library
functions that take a `PIO` as their first argument are exposed as methods on
the `PIO` class.

- `PIO:sm(index) -> SM`\
  Return the state machine with the given index.

- `PIO:get_index() -> integer`\
  Return the instance number of the peripheral.

- `PIO:regs() -> pointer`\
  Return a pointer to the base of the peripheral registers (`PIOx_BASE`).

- `PIO:gpio_init(pin)`\
  Set up the function select for the GPIO to use output from the PIO instance.

- `PIO:can_add_program(program) -> boolean`\
  `PIO:can_add_program_at_offset(program, offset) -> boolean`
  Return `true` iff `program` can (at the time of the call) be loaded onto the
  PIO instance.

- `PIO:add_program(program) -> integer`\
  `PIO:add_program_at_offset(program, offset)`\
  Load `program` onto the PIO instance, returning the offset at which the
  program was loaded.

- `PIO:remove_program(program, offset)`\
  Remove `program` from the PIO instance's instruction memory.

- `PIO:clear_instruction_memory()`\
  Clear all of the PIO instance's instruction memory.

- `PIO:set_sm_mask_enabled(mask, enabled)`\
  Enable or disable multiple state machines.

- `PIO:restart_sm_mask(mask)`\
  Restart multiple state machines with a known state.

- `PIO:clkdiv_restart_sm_mask(mask)`\
  Restart multiple state machines' clock dividers from a phase of 0.

- `PIO:enable_sm_mask_in_sync(mask)`\
  Enable multiple state machines, synchronizing their clock dividers.

- `PIO:interrupt_get(num) -> boolean`\
  `PIO:interrupt_get_mask(mask) -> integer`\
  Determine if one or more state machine interrupts are pending.

- `PIO:interrupt_clear(num)`\
  `PIO:interrupt_clear_mask(mask)`\
  Clear one or more state machine interrupts.

- `PIO:claim_sm_mask(mask)`\
  Mark multiple state machines as used.

- `PIO:claim_unused_sm(required = true) -> integer`\
  Claim an unused state machine. Returns the index of the claimed state machine.

- `PIO:wait_irq(mask) -> integer` *[yields]*\
  Wait for one of the state machine interrupts indicated by the mask to be set.
  Returns the interrupts that are currently pending, masked by `mask`. This
  function blocks without yielding if the mask contains interrupts that aren't
  routed to the system IRQs.

- `PIO:enable_irq(mask, enable)`\
  [Enable or disable](core.md#irq-enablers) the PIO IRQ handler (`PIOx_IRQ_n`)
  for a set of interrupt sources. `mask` is a bit mask of `pis_*` interrupt
  source numbers. The module uses IRQ 0 on core 0 and IRQ 1 on core 1.

## `hardware.pio.asm`

**Module:** [`hardware.pio.asm`](../lib/pico/hardware.pio.asm.lua),
build target: `mlua_mod_hardware.pio.asm`,
tests: [`hardware.pio.asm.test`](../lib/pico/hardware.pio.asm.test.lua)

This module provides an assembler for PIO programs. It allows creating PIO
programs in pure Lua, using a syntax very close to PIO assembly (thanks to some
advanced metatable magic). It is similar to the
[`rp2.asm_pio()`](https://docs.micropython.org/en/latest/library/rp2.html#rp2.asm_pio)
decorator in MicroPython, with similar but slightly more intuitive
[syntax](https://docs.micropython.org/en/latest/library/rp2.html#pio-assembly-language-instructions).

- `assemble(fn) -> Program`\
  Assemble the program defined by the function `fn`.

### `Program`

- `[1], ...: integer`\
  The program's instructions, as 16-bit integers.

- `origin: integer`\
  When specified, the program must be loaded at this offset in PIO instruction
  memory.

- `labels: table`\
  The public labels defined by the program.

- `wrap_target: integer`\
  `wrap: integer`\
  The wrap target and wrap address of the program.

- `Program:config([offset]) -> Config`\
  Return a default configuration for the program when loaded at the given
  offset, with the wrap and side-set configuration already set. `offset` is
  optional if `origin` is set.

### PIO program syntax

A PIO program is defined as a `function(_ENV)`. Within the function, the program
is constructed through calls to directive and instruction functions. The
[test suite](../lib/pico/hardware.pio.asm.test.lua) has a number of example
programs.

Labels have the following forms:

- `<label>:`\
  Defines a private label named `label`.

- `public(<label>):`\
  Defines a public label named `label`. Public labels are avilable at the key
  `labels` in the assembled program. Note that when both a private and a public
  label are defined for the same instruction, the private label must be
  specified first.

Directives do not generate instructions, but specify configuration for the
program.

- `origin(offset)`\
  Specify the PIO instruction memory offset at which the program must be loaded.
  The offset is available at the key `origin` in the assembled program.

- `side_set(count, ...)`\
  Indicate the number of side-set bits to be used. An optional `opt` argument
  specifies that side-setting is optional. It is set automatically if at least
  one instruction doesn't have a `side()`. An optional `pindirs` argument
  specifies that side-set values should be applied to the pin directions.

- `wrap_target()`\
  Indicate that program wrapping should jump to the instruction just after the
  directive. The wrap target is available at the key `wrap_target` of the
  assembled program.

- `wrap()`\
  Indicate that the program should wrap after the instruction just prior to the
  directive. The wrap instruction offset is available at the key `wrap` of the
  assembled program.

Instructions are added by calling the functions below, optionally followed by
`side(value)` to specify a side-set and / or `(delay)` to specify a delay for
the instruction.

- `word(instr, [label])`\
  Add an arbitrary 16-bit instruction. If specified, the offset of `label` is
  ORed into the instruction.

- `nop()`\
  Add a `mov(y, y)` insruction, which does nothing.

- `jmp([cond], label)`\
  Add a `JMP` instruction. The optional condition can be one of `~x`, `x_dec`,
  `~y`, `y_dec`, `x~y`, `pin` or `~osre`.

- `wait(polarity, src, num)`\
  Add a `WAIT` instruction. The source can be one of `gpio`, `pin` or `irq`.
  For the `irq` source, a relative IRQ number can be specified as `rel(num)`.

- `in_(src, count)`\
  Add an `IN` instruction. The source can be one of `pins`, `x`, `y`, `null`,
  `isr` or `osr`.

- `out(dest, count)`\
  Add an `OUT` instruction. The destination can be one of `pins`, `x`, `y`,
  `null`, `pindirs`, `pc`, `isr` or `exec`.

- `push(...)`\
  Add a `PUSH` instruction. The optional flags can be any of `iffull`, `block`
  and `noblock`.

- `pull(...)`\
  Add a `PULL` instruction. The optional flags can be any of `ifempty`, `block`
  and `noblock`.

- `mov(dest, src)`\
  Add a `MOV` instruction. The destination can be one of `pins`, `x`, `y`,
  `exec`, `pc`, `isr` or `osr`. The source can be one of `pins`, `x`, `y`,
  `null`, `status`, `isr` or `osr`. The source can optionally be prefixed with
  `~` to indicate that it should be inverted (bitwise complement) or `#` to
  indicate that it should be bit-reversed.

- `irq([mode], num)`\
  Add a an `IRQ` instruction. The optional mode can be one of `wait` or `clear`.
  A relative IRQ number can be specified as `rel(num)`.

- `set(dest, data)`\
  Add a `SET` instruction. The destination can be one of `pins`, `x`, `y` or
  `pindirs`.

## `hardware.pll`

**Library:** [`hardware_pll`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pll),
header: [`hardware/pll.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pll/include/hardware/pll.h),
sources: [`hardware_pll`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pll)\
**Module:** [`hardware.pll`](../lib/pico/hardware.pll.c),
build target: `mlua_mod_hardware.pll`,
tests: [`hardware.pll.test`](../lib/pico/hardware.pll.test.lua)

## `hardware.pwm`

**Library:** [`hardware_pwm`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pwm),
header: [`hardware/pwm.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pwm/include/hardware/pwm.h),
sources: [`hardware_pwm`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_pwm)\
**Module:** [`hardware.pwm`](../lib/pico/hardware.pwm.c),
build target: `mlua_mod_hardware.pwm`,
tests: [`hardware.pwm.test`](../lib/pico/hardware.pwm.test.lua)

This module defines the `hardware.pwm.Config` class, which exposes PWM
configuration functionality. All library functions that take an
`pwm_config*` as a first argument are exposed as methods on the `Config` class.

- `NUM_SLICES: integer`\
  The number of PWM slices available on the target.

- `regs([slice]) -> pointer`\
  When `slice` is absent or `nil`, return a pointer to the base of the
  peripheral registers (`PWM_BASE`). Otherwise, return a pointer to the base
  of the registers for the given slice.

- `set_irq_handler(handler) -> Thread`\
  Set a PWM wrap IRQ handler. Returns the
  [event handler thread](core.md#callbacks).

  - `handler(slice_mask)`\
    The handler to be called on PWM IRQ events.

  The handler can be removed either by killing the returned thread, or by
  calling `set_irq_handler()` with a `nil` handler.

## `hardware.regs.*`

**Library:** `hardware_regs`,
sources: [`hardware_regs`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs)\
**Modules:** `hardware.regs.*`,
build targets: `mlua_mod_hardware.regs.*`,
tests: [`hardware.regs.test`](../lib/pico/hardware.regs.test.lua)

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

- `hardware.regs.addressmap`: [`hardware/regs/addressmap.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/addressmap.h)
  - The constants that specify addresses are [pointers](core.md#pointers).
- `hardware.regs.adc`: [`hardware/regs/adc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2040/hardware_regs/include/hardware/regs/adc.h)
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
**Module:** [`hardware.resets`](../lib/pico/hardware.resets.c),
build target: `mlua_mod_hardware.resets`,
tests: [`hardware.resets.test`](../lib/pico/hardware.resets.test.lua)

The reset bitmasks are avaialble in the `hardware.regs.resets` module as
`RESET_*_BITS`.

- `unreset_block_wait(bits)`\
  This function blocks without yielding.

## `hardware.rtc`

**Library:** [`hardware_rtc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_rtc),
header: [`hardware/rtc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_rtc/include/hardware/rtc.h),
sources: [`hardware_rtc`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_rtc)\
**Module:** [`hardware.rtc`](../lib/pico/hardware.rtc.c),
build target: `mlua_mod_hardware.rtc`,
tests: [`hardware.rtc.test`](../lib/pico/hardware.rtc.test.lua)

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
**Module:** [`hardware.spi`](../lib/pico/hardware.spi.c),
build target: `mlua_mod_hardware.spi`,
tests: [`hardware.spi.test`](../lib/pico/hardware.spi.test.lua)

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

- `SPI:regs() -> pointer`\
  Return a pointer to the base of the peripheral registers (`SPIx_BASE`).

- `SPI:write_read_blocking(src) -> string` *[yields]*\
  `SPI:write_blocking(src)` *[yields]*\
  `SPI:read_blocking(tx_data, len) -> string` *[yields]*\
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
**Module:** [`hardware.sync`](../lib/pico/hardware.sync.c),
build target: `mlua_mod_hardware.sync`,
tests: [`hardware.sync.test`](../lib/pico/hardware.sync.test.lua)

> [!NOTE]
> Spin lock functionality isn't exposed to Lua, because spin locks are supposed
> to be held for a short duration only, which can't be achieved from Lua.

## `hardware.timer`

**Library:** [`hardware_timer`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_timer),
header: [`hardware/timer.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_timer/include/hardware/timer.h),
sources: [`hardware_timer`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_timer)\
**Module:** [`hardware.timer`](../lib/pico/hardware.timer.c),
build target: `mlua_mod_hardware.timer`,
tests: [`hardware.timer.test`](../lib/pico/hardware.timer.test.lua)

- `time_us() -> integer`\
  Return the low-order bits of the current
  [absolute time](mlua.md#absolute-time) that fit a Lua integer.

- `time_us_32() -> integer`\
  Return the 32 lower-order bits of the current
  [absolute time](mlua.md#absolute-time).

- `time_us_64() -> Int64`\
  Return the current [absolute time](mlua.md#absolute-time).

- `busy_wait_us_32(duration)`\
  Busy wait wasting cycles for the given (32-bit) number of microseconds.
  `duration` must be an integer.

- `busy_wait_us(duration)`\
  Busy wait wasting cycles for the given (64-bit) number of microseconds.
  `duration` must be an integer or an `Int64`.

- `busy_wait_ms(duration)`\
  Busy wait wasting cycles for the given number of milliseconds. `duration` must
  be an integer.

- `busy_wait_until(time)`\
  Busy wait wasting cycles until the given
  [absolute time](mlua.md#absolute-time) is reached.

- `time_reached(time) -> boolean`\
  Return true iff the given [absolute time](mlua.md#absolute-time) has been
  reached.

- `claim(alarm)`\
  Cooperatively claim a hardware timer. Panics if the timer is already claimed.

- `claim_unused() -> integer`\
  Cooperatively claim an unused hardware timer.

- `unclaim(alarm)`\
  Cooperatively release the claim of a hardware timer.

- `is_claimed(alarm) -> boolean`\
  Return `true` iff the given hardware timer is claimed.

- `set_callback(alarm, callback) -> Thread`\
  Set the callback for the hardware timer `alarm`. Returns the
  [event handler thread](core.md#callbacks).

  - `callback(alarm)`\
    The callback to be called when the timer triggers.

  The callback can be removed either by killing the returned thread, or by
  calling `set_callback()` with a `nil` callback.

- `set_target(alarm, time) -> boolean`\
  Set the current target [absolute time](mlua.md#absolute-time) for a hardware
  timer. Returns `true` iff the target was missed, i.e. it was in the past or
  occurred before the target could be set.

- `cancel(alarm)`\
  Cancel a hardware timer, if it was set.

- `force_irq(alarm)`\
  Force an IRQ for a hardware timer.

## `hardware.uart`

**Library:** [`hardware_uart`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_uart),
header: [`hardware/uart.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_uart/include/hardware/uart.h),
sources: [`hardware_uart`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_uart)\
**Module:** [`hardware.uart`](../lib/pico/hardware.uart.c),
build target: `mlua_mod_hardware.uart`,
tests: [`hardware.uart.test`](../lib/pico/hardware.uart.test.lua)

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

- `UART:regs() -> pointer`\
  Return a pointer to the base of the peripheral registers (`UARTx_BASE`).

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

- `UART:is_readable_within_us(timeout) -> boolean` *[yields]*\
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
**Module:** [`hardware.vreg`](../lib/pico/hardware.vreg.c),
build target: `mlua_mod_hardware.vreg`,
tests: [`hardware.vreg.test`](../lib/pico/hardware.vreg.test.lua)

## `hardware.watchdog`

**Library:** [`hardware_watchdog`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_watchdog),
header: [`hardware/watchdog.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_watchdog/include/hardware/watchdog.h),
sources: [`hardware_watchdog`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_watchdog)\
**Module:** [`hardware.watchdog`](../lib/pico/hardware.watchdog.c),
build target: `mlua_mod_hardware.watchdog`,
tests: [`hardware.watchdog.test`](../lib/pico/hardware.watchdog.test.lua)

## `hardware.xosc`

**Library:** [`hardware_xosc`](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_xosc),
header: [`hardware/xosc.h`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_xosc/include/hardware/xosc.h),
sources: [`hardware_xosc`](https://github.com/raspberrypi/pico-sdk/blob/master/src/rp2_common/hardware_xosc)\
**Module:** [`hardware.xosc`](../lib/pico/hardware.xosc.c),
build target: `mlua_mod_hardware.xosc`,
tests: [`hardware.xosc.test`](../lib/pico/hardware.xosc.test.lua)
