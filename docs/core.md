# Core functionality

## Binding conventions

There is a fairly obvious mapping from the C library name to the corresponding
Lua module. For example, the `hardware_i2c` library is wrapped by the
`hardware.i2c` module.

The constants and functions exported by C libraries are exposed as module- or
class-level symbols with the same names, with obvious prefixes stripped when
they would cause stuttering. For example:

- `PICO_UART_ENABLE_CRLF_SUPPORT` => `hardware.uart.ENABLE_CRLF_SUPPORT`
- `UART_PARITY_NONE` => `hardware.uart.PARITY_NONE`
- `uart_set_baudrate()` => `hardware.uart.Uart.set_baudrate()`

Function arguments are kept in the same order as their C counterpart, except for
out arguments, which map to multiple return values. Function argument types are
mapped as follows:

C type                | Lua type
--------------------- | ------------
`bool`                | `boolean`
integer (<=32 bits)   | `integer`
`int64_t`, `uint64_t` | `mlua.int64`
`float`, `double`     | `number`
`char*`               | `string`
pointer + size        | `string`
pointer to struct     | `userdata`
`absolute_time_t`     | `mlua.int64`

As a convenience, `bool` function arguments accept `0` as `false`.

## Lua modules

TODO: Document

## Function metatable

TODO: Document

## Read-only tables

TODO: Document

## Events

TODO: Document

### IRQ enablers

IRQ enabler functions set up an IRQ handler and one or more [events](#events).
Their `enable` argument can take the following values:

-   Missing, `true`, `nil`, negative `integer`: Set up an exclusive IRQ handler
    and enable the IRQ.
-   Non-negative `integer`: Set up a shared IRQ handler with the given priority
    and enable the IRQ. The priority must be in the range
    [`hardware.irq.SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY`;
    `hardware.irq.SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY`], i.e. `[0; 0xff]`.
-   `false`: Disable the IRQ and remove the IRQ handler.

### Callbacks

TODO: Document
