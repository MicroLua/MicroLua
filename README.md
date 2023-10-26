# MicroLua

MicroLua allows **programming the
[RP2040 microcontroller](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html)
in [Lua](https://www.lua.org/)**. It packages the latest, pristine Lua
interpreter with bindings for the
[pico-sdk](https://github.com/raspberrypi/pico-sdk).

MicroLua is licensed under the [MIT](LICENSE.md) license.

 - Mailing list:
   [microlua@freelists.org](https://www.freelists.org/list/microlua)

## Why

 - Lua is a small, embeddable language. It is easy to learn and reasonably fast.

 - The RP2040 is a small, powerful microcontroller with a nice set of
   peripherals and an active developer community. Besides the official
   [Pico](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#technical-specification)
   and
   [Pico W](https://www.raspberrypi.com/documentation/microcontrollers/raspberry-pi-pico.html#raspberry-pi-pico-w-and-pico-wh)
   boards from
   [Raspberry Pi](https://www.raspberrypi.com/), a variety of cheap modules in
   various shapes and configurations are readily available for purchase.

I had been wanting to play with Lua for a very long time, without having a
concrete use case for it. Similarly, I wanted to explore the RP2040 but didn't
have a concrete project for it. MicroLua served as an excuse to get started with
both.

## Features

 - **Pristine, unpatched Lua interpreter:** MicroLua runs the latest, unmodified
   Lua 5.4.x interpreter, imported as a git submodule. All customization is done
   through [`luaconf.h`](core/luaconf.in.h).
 - **Per-core interpreter instances:** MicroLua runs a separate Lua interpreter
   in each core. They don't share state except through C libraries.
 - **Cooperative multithreading through Lua coroutines:** MicroLua implements
   cooperative threads as coroutines. This enables multitasking without the need
   for locks. Blocking library calls (e.g. `pico.time.sleep_us()`) yield to
   other threads.
 - **Thin bindings to C libraries:** MicroLua exposes a growing subset of the
   functionality provided by the pico-sdk. The bindings are designed with a
   direct and consistent [mapping](#binding-conventions) to their underlying C
   implementation.
 - **Comprehensive suite of unit tests:** They not only test the binding layer,
   but when possible also the underlying functionality of the pico-sdk.

## Building

```shell
# Configure the location of the pico-sdk. Adjust to your setup.
$ export PICO_SDK_PATH="${HOME}/pico-sdk"

# Clone the repository and initialize submodules.
$ git clone https://github.com/MicroLua/MicroLua.git
$ cd MicroLua
$ git submodule update --init

# Connect a Picoprobe to the target, on the UART and optionally on the debug
# port. Then view the Picoprobe's UART connection in a separate terminal.
$ tools/term /dev/ttyACM0

# Build the unit tests.
$ cmake -s . -B build -DPICO_BOARD=pico
$ make -j9 -C build/lib

# Flash the target with a Picoprobe. Alternatively, start the target in BOOTSEL
# mode and copy build/lib/mlua_tests.uf2 to its drive.
$ tools/flash build/lib/mlua_tests.elf
```

## Examples

The [MicroLua-examples](https://github.com/MicroLua/MicroLua-examples)
repository contains example programs that demonstrate how to use the various
features of the RP2040 from Lua.

Here's the `blink` example in MicroLua, a translation of the
[`blink`](https://github.com/raspberrypi/pico-examples/tree/master/blink)
example from the [`pico-examples`](https://github.com/raspberrypi/pico-examples)
repository.

```lua
-- main.lua
_ENV = mlua.Module(...)

local gpio = require 'hardware.gpio'
local pico = require 'pico'
local time = require 'pico.time'

function main()
    local LED_PIN = pico.DEFAULT_LED_PIN
    gpio.init(LED_PIN)
    gpio.set_dir(LED_PIN, gpio.OUT)
    while true do
        gpio.put(LED_PIN, 1)
        time.sleep_ms(250)
        gpio.put(LED_PIN, 0)
        time.sleep_ms(250)
    end
end
```

```cmake
# CMakeLists.txt
mlua_add_lua_modules(mlua_examples_blink_main main.lua)
target_link_libraries(mlua_examples_blink_main INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_pico
    mlua_mod_pico_time
)

add_executable(mlua_examples_blink)
target_link_libraries(mlua_examples_blink PRIVATE
    mlua_examples_blink_main
    pico_stdlib
)
pico_add_extra_outputs(mlua_examples_blink)
```

To build the example:

```shell
# Configure the locations of the pico-sdk and MicroLua. Adjust to your setup.
$ export PICO_SDK_PATH="${HOME}/pico-sdk"
$ export MLUA_PATH="${HOME}/MicroLua"

# Clone the MicroLua-examples repository.
$ git clone https://github.com/MicroLua/MicroLua-examples.git
$ cd MicroLua-examples

# Build the "blink" example.
$ cmake -s . -B build -DPICO_BOARD=pico
$ make -j9 -C build/blink

# Flash the target with a Picoprobe. Alternatively, start the target in BOOTSEL
# mode and copy build/blink/mlua_examples_blink.uf2 to its drive.
$ "${MLUA_PATH}/tools/flash" build/blink/mlua_examples_blink.elf
```

## Documentation

### Binding conventions

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

| C type                | Lua type     |
| --------------------- | ------------ |
| `bool`                | `boolean`    |
| integers (<=32 bits)  | `integer`    |
| `int64_t`, `uint64_t` | `mlua.int64` |
| `float`, `double`     | `number`     |
| `char*`               | `string`     |
| pointers              | `userdata`   |

As a convenience, `bool` function arguments accept `0` as `false`.

### Modules

 - [`hardware.*`](docs/hardware.md): Bindings for the `hardware_*` libraries
   of the pico-sdk.
 - [`mlua.*`](docs/mlua.md): MicroLua libraries.
 - [`pico.*`](docs/pico.md): Bindings for the `pico_*` libraries of the
   pico-sdk.

<!-- TODO: Document how to embed MicroLua into a C application -->
<!-- TODO: Document how to write a MicroLua module in C -->
<!-- TODO: Document ROM-based tables -->
<!-- TODO: Document config knobs -->

## Contributing

While I'm happy to accept bug reports, feature requests and other suggestions on
the [mailing list](https://www.freelists.org/list/microlua), I am not actively
looking for code contributions. In particular, **please do not send pull
requests** on Github. MicroLua is developed in a private
[Mercurial](https://www.mercurial-scm.org/) repository, which is mirrored to
Github.

## FAQ

### Why Lua? Why the RP2040?

Both Lua and the RP2040 evolve at a velocity that can be followed by a single
developer in their spare time. Lua is developed by a small team: the language
itself evolves very slowly and the interpreter has a couple of minor releases
each year. Similarly, the RP2040 is developed by a small company, and a new chip
may be released every few years.

Contrast this with e.g. Python: the language evolves rather quickly nowadays,
and the interpreter gets contributions from dozens (hundreds? thousands?) of
developers. Similarly, large semiconductor companies release new chips and
variants every year. Developing for these targets is a game of catch-up that I
don't want to play.

### Can MicroLua support other microcontroller families?

No, that's an explicit non-goal. Supporting multiple microcontroller families
inevitably leads to either limiting features to the common denominator, or
introducing a complex configuration system.

Moreover, MicroLua integrates directly with the build system of the pico-sdk,
which is strongly tied to the RP2040. Supporting other microcontrollers would
require a different build system.

MicroLua will likely support later RP devices if / when they get released, but
until then, the RP2040 is the only supported target.

### How does MicroLua compare to other Lua projects for the Pico?

 - [picolua](https://github.com/kevinboone/luapico) is based on a patched Lua
   5.4 interpreter, and aims to provide a full embedded development environment,
   including a shell and an editor. It exposes a limited subset of Pico-specific functionality.

   MicroLua uses an unpatched Lua interpreter at the latest stable version, and
   aims to expose most of the functionality provided by the pico-sdk through a
   thin binding layer.
