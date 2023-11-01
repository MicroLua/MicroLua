# `mlua.*` modules

This page describes the modules specific to MicroLua.

The test modules can be useful as usage examples.

## Globals

- `_VERSION: string = LUA_VERSION`\
  `_RELEASE: string = LUA_RELEASE`

- `mlua`\
  The `mlua` module is auto-loaded on startup and made available as a global.
  This simplifies module definitions.

## `mlua`

**Module:** [`mlua`](../lib/mlua.lua),
tests: [`mlua.test`](../lib/mlua.test.lua)

- `Module()`\
  Create a new empty module.

## `mlua.config`

**Module:** `mlua.config` (auto-generated),
tests: [`mlua.config.test`](../lib/mlua.config.test.lua)

This module is auto-generated when `mlua_add_config_module()` is called for
a CMake build target, and contains the symbols defined by `mlua_target_config()`
calls. Symbol definitions have the format `{name}:{type}={value}`, where
`{type}` is one of `boolean`, `integer`, `number` or `string`.

**Example:**

```cmake
mlua_target_config(example_target
    my_bool:boolean=true
    my_int:integer=42
    my_num:number=123.456
    my_str:string=\"some-string\"
)
```

## `mlua.event`

**Module:** [`mlua.event`](../lib/mlua.event.c)

This module provides the core event handling and dispatch functionality.

- `dispatch(time)`\
  Wait for events to fire or the given time to pass. This function is used by
  the thread scheduler.

## `mlua.int64`

**Module:** [`mlua.int64`](../lib/mlua.int64.c),
tests: [`mlua.int64.test`](../lib/mlua.int64.test.lua)

This module provides a 64-bit signed integer type (`mlua.int64`). When
`lua_Integer` is a 32-bit integer, `int64` is a full userdata with all the
relevant metamethods, supporting mixed-type operations with `integer` (but no
automatic promotion to `number`). When `lua_Integer` is a 64-bit integer,
`int64` is an alias for `integer`.

> [!IMPORTANT]
> Mixed-type equality comparisons (`==`, `~=`) involving `int64` values do not
> work correctly, because Lua only calls the `__eq` metamethod if both values
> are either tables or full userdata. Use `eq()` instead if the arguments may
> be primitive types.

- `int64(value) -> int64 | nil`\
  Cast `value` to an `int64`. `value` can have the following types:

  - `boolean`: `false -> 0`, `true -> 1`
  - `integer`: Sign-extends `value` to an `int64`. When `lua_Integer` is a
    32-bit integer, an optional second argument provides the high-order 32 bits.
  - `number`: Fails if `value` cannot be represented exactly as an `int64`.
  - `string`: Parse the value from a string. Accepts an optional `base` argument
    (default: 0). When the base is 0, it is inferred from the value prefix
    (`0x`, `0X`: 16, `0o`: 8, `0b`: 2, otherwise: 10).

- `max: int64`\
  The maximum value that an `int64` can hold (`2^63-1`)

- `min: int64`\
  The minimum value that an `int64` can hold (`-2^63`).

- `ashr(value, num) -> int64`\
  Returns the result of performing an arithmetic (i.e. sign-extending) right
  shift of `value` by `num` bits.

- `eq(lhs, rhs) -> boolean`\
  Compares `lhs` and `rhs` for equality. Works correctly for mixed-type
  comparisons.

- `hex(value) -> string`\
  Return a hexadecimal representation of `value`.

- `tointeger(value) -> integer | nil`\
  Convert `value` to an `integer`. Fails if `value` cannot be represented
  exactly as an `integer`.

- `tonumber(value) -> number | nil`\
  Convert `value` to a `number`. Fails if `value` cannot be represented
  exactly as a `number`.

- `ult(lhs, rhs) -> boolean`\
  Return true iff `lhs` is less than `rhs` when they are compared as unsigned
  64-bit integers.

## `mlua.io`

**Module:** [`mlua.io`](../lib/mlua.io.lua),
tests: [`mlua.io.test`](../lib/mlua.io.test.lua)

This module provides helpers for input / output processing.

> [!IMPORTANT]
> Output functions block without yielding if the output buffer for `stdout` is
> full.

- `read(count) -> string | nil`\
  Read at least one and at most `count` characters from `stdin`. Uses
  `pico.stdio.read()` if the module is available, or blocks without yielding
  if no data is available.

- `write(data) -> integer | nil`\
  Write data to `stdout`, and return the number of characters written.

- `printf(format, ...) -> integer | nil`\
  Format the arguments with `string:format()` and write the result to `stdout`.

- `fprintf(out, format, ...) -> integer | nil`\
  Format the arguments with `string:format()` and write the result to `out`.

- `ansi(s) -> string`\
  Substitute tags of the form `@{...}` with the corresponding ANSI escape codes.
  See the module for supported tags.

- `aprintf(format, ...) -> integer | nil`\
  Substitute tags in `format`, format the arguments with `string:format()` and
  output the result to `stdout`.

- `afprintf(out, format, ...) -> integer | nil`\
  Substitute tags in `format`, format the arguments with `string:format()` and
  output the result to `out`.

### `Buffer`

The `Buffer` type records writes and allows replaying them on another stream.

- `Buffer:is_empty() -> boolean`\
  Return true iff the buffer holds no data.

- `Buffer:write(data)`\
  Write data to the buffer.

- `Buffer:replay(w)`\
  Replay the recorded writes to `w`.

- `tostring(Buffer) -> string`\
  Return the content of the buffer as a string.

## `mlua.mem`

**Module:** [`mlua.mem`](../lib/mlua.mem.c),
tests: [`mlua.mem.test`](../lib/mlua.mem.test.lua)

This module provides functionality to access raw memory (ROM, RAM).

> [!IMPORTANT]
> This module must not be used to access hardware registers. Use the
> [`hardware.base`](hardware.md#hardwarebase) module instead.

- `read(address, size) -> string`\
  Read `size` bytes in memory starting at `address`.

- `write(address, data)`\
  Write `data` to memory starting at `address`.

- `alloc(size) -> Buffer`\
  Allocate a memory buffer of the given size.

### `Buffer`

The `Buffer` type holds a fixed-size memory buffer.

- `#Buffer -> integer`\
  Return the size of the buffer.

- `Buffer:addr() -> integer`\
  Return the address of the buffer in memory.

- `Buffer:clear(value = 0)`\
  Clear the buffer by setting all bytes to `value`.

- `Buffer:read(offset = 0, len = size - offset) -> string`\
  Read `len` bytes from the buffer, starting at `offset`.

- `Buffer:write(data, offset = 0)`\
  Write `data` to the buffer, starting at `offset`.

## `mlua.oo`

**Module:** [`mlua.oo`](../lib/mlua.oo.lua),
tests: [`mlua.oo.test`](../lib/mlua.oo.test.lua)

## `mlua.stdio`

**Module:** [`mlua.stdio`](../lib/mlua.stdio.c),
tests: [`mlua.stdio.test`](../lib/mlua.stdio.test.lua)

## `mlua.testing`

**Module:** [`mlua.testing`](../lib/mlua.testing.lua),
tests: [`mlua.testing.test`](../lib/mlua.testing.test.lua)

- [`mlua.testing.clocks`](../lib/mlua.testing.clocks.lua): Helpers for testing
  clock-related functionality.
- [`mlua.testing.i2c`](../lib/mlua.testing.i2c.lua): Helpers for testing I2C
  functionality.
- [`mlua.testing.stdio`](../lib/mlua.testing.stdio.lua): Helpers for testing
  stdio functionality.
- [`mlua.testing.uart`](../lib/mlua.testing.uart.lua): Helpers for testing UART
  functionality.

## `mlua.thread`

**Module:** [`mlua.thread`](../lib/mlua.thread.lua),
tests: [`mlua.thread.test`](../lib/mlua.thread.test.lua)

## `mlua.util`

**Module:** [`mlua.util`](../lib/mlua.util.lua),
tests: [`mlua.util.test`](../lib/mlua.util.test.lua)
