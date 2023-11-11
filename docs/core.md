# Core functionality

<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

<!-- TODO: Document config knobs -->
<!-- TODO: Document how to embed MicroLua into a C application -->
<!-- TODO: Document how to write a MicroLua module in C -->

## MicroLua project

Here's how to get started with a new project based on MicroLua:

1. Create a file `main.lua` containing a `main()` function.

    ```lua
    _ENV = mlua.Module(...)

    function main()
      print("Hello, world!")
    end
    ```

2.  Create a top-level `CMakeLists.txt` for the project.

    ```cmake
    # Define the project and initialize MicroLua.
    cmake_minimum_required(VERSION 3.20)

    include(mlua_import.cmake)

    project(my_project C CXX ASM)
    set(CMAKE_C_STANDARD 11)
    set(CMAKE_CXX_STANDARD 17)

    mlua_init()

    # Add a library for the main module.
    mlua_add_lua_modules(my_project_main main.lua)
    target_link_libraries(my_project_main INTERFACE
        mlua_mod_mlua_stdio
    )

    # Add the executable.
    add_executable(my_project_hello)
    target_link_libraries(my_project_hello PRIVATE
        my_project_main
        pico_stdlib
    )
    pico_add_extra_outputs(my_project_hello)
    ```

3.  Copy the file [`mlua_import.cmake`](../mlua_import.cmake) from the MicroLua
    repository to the same directory as the top-level `CMakeLists.txt` of the
    project.

4.  Build the project.

    ```shell
    # Adjust the paths for your setup.
    $ export PICO_SDK_PATH="${HOME}/pico-sdk"
    $ export MLUA_PATH="${HOME}/MicroLua"

    # Build the executable.
    $ cmake -S . -B build -DPICO_BOARD=pico
    $ make -j9 -C build

    # This generates the executable files build/my_project_hello.elf and
    # build/my_project_hello.uf2, which can be flashed onto a target.
    ```

## Lua modules

A Lua module is a `.lua` file with the following line at the very top:

```lua
_ENV = mlua.Module(...)
```

This creates a new module, registers it with the name given to the `require()`
call, and assigns it to the block's environment. The block can then define
the module contents.

- Local variables and functions are private to the module.
- Non-local variables and functions are exported.
- Lookups of undefined symbols are forwarded to the global environment `_G`.
- Lookups of undefined symbols in the global environment raise an error.

The `.lua` file must be registered as a library in `CMakeFile.txt`, and its
dependencies must be declared:

```cmake
mlua_add_lua_modules(mod_example example.lua)
target_link_libraries(mod_example INTERFACE
    mlua_mod_hardware_adc
    mlua_mod_hardware_gpio
)
```

### Fennel

MicroLua supports writing modules in [Fennel](https://fennel-lang.org/), by
transpiling Fennel sources to Lua. Simply use `mlua_add_fnl_modules()` instead
of `mlua_add_lua_modules()`. There are a few examples in the
[MicroLua-examples](https://github.com/MicroLua/MicroLua-examples/blob/master/fennel)
repository.

## Function metatable

The MicroLua runtime sets a metatable on the `function` type with a `__close`
metamethod that calls the function. This makes it easy to define deferreds, e.g.
for reliable resource management.

```lua
do
    local save = sync.save_and_disable_interrupts()
    local done<close> = function() sync.restore_interrupts(save) end
    flash.range_program(offs, data)
end  -- Interrupts are restored here, even if range_program() raises an error
```

## Read-only tables

Read-only tables reduce the RAM usage of tables where the keys are known at
compile-time and mostly immutable, for example module or class tables. They are
implemented using
[minimal perfect hashing](https://en.wikipedia.org/wiki/Perfect_hash_function#Minimal_perfect_hash_function)
with omitted key values. Table definitions (`MLUA_SYMBOLS()`) are parsed out of
C modules at compile-time to generate the hash functions. The hash function and
the table *values* are stored in flash, while the table *keys* aren't stored at
all. At runtime, lookups are performed in the `__index` metamethod by computing
the hash of the key and looking up the corresponding value.

The perfect hash functions are computed using the CHM92 algorithm, as described
in:

> [An optimal algorithm for generating minimal perfect hash functions](https://citeseerx.ist.psu.edu/doc/10.1.1.51.5566)\
> Z. J. Czech, G. Havas and B. S. Majewski\
> Information Processing Letters, 43(5):257-264, 1992

Not storing table keys massively reduces flash usage, and makes it possible to
include huge modules like [`hardware.regs.io_bank0`](hardware.md#hardwareregs),
which wouldn't even fit in flash otherwise. There is one downside, however:

> [!IMPORTANT]
> **Lookups of string keys in read-only tables always succeed.** When looking up
> a key that wasn't present in the original table, an arbitrary, other value
> from the table is returned.

This is usually not an issue for module and class tables, but it means that it's
not possible to check for the presence of a key, and typos will cause undefined
behavior, making debugging more difficult. To alleviate the latter during
development, the `MLUA_SYMBOL_HASH_DEBUG` compile definition can be set to `1`
for a build target, which causes table keys to be stored in flash and checked at
runtime. This significantly increases flash usage and degrades lookup
performance, so it shouldn't be left enabled in a release build.

```cmake
target_compile_definitions(example_target PRIVATE
    MLUA_SYMBOL_HASH_DEBUG=1
)
```

## Binding conventions

There is a fairly obvious mapping from the C library name to the corresponding
Lua module. For example, the `hardware_i2c` library is wrapped by the
`hardware.i2c` module.

The constants and functions exported by C libraries are exposed as module- or
class-level symbols with the same names, with obvious prefixes stripped when
they would cause stuttering. For example:

- `PICO_UART_ENABLE_CRLF_SUPPORT` => `hardware.uart.ENABLE_CRLF_SUPPORT`
- `UART_PARITY_NONE` => `hardware.uart.PARITY_NONE`
- `uart_set_baudrate()` => `hardware.uart.UART.set_baudrate()`

Function arguments are kept in the same order as their C counterpart, except for
out arguments, which map to multiple return values. Function argument types are
mapped as follows:

C type                | Lua type
--------------------- | ----------
`bool`                | `boolean`
integer (<=32 bits)   | `integer`
`int64_t`, `uint64_t` | `Int64`
`float`, `double`     | `number`
`char*`               | `string`
pointer + size        | `string`
pointer to struct     | `userdata`
`absolute_time_t`     | `Int64`

As a convenience, `bool` function arguments accept `0` as `false`.

## Events

Events are a mechanism to bridge low-level IRQ handlers and callbacks with
high-level Lua code. Each event has an associated set of watching threads. When
an event is triggered, the threads watching the event are moved to the active
queue at the next context switch. This allows threads to register interest in an
event and suspend themselves, and to be resumed when the event happens.

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

Pico SDK functions that accept a callback are implemented using threads. A C
callback is set up to trigger an event, and the thread watches the event and
calls the Lua callback when the event triggers.
