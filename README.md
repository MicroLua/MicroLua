<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

# MicroLua - Lua for the RP2040 microcontroller

MicroLua allows **programming the
[RP2040 microcontroller](https://www.raspberrypi.com/documentation/microcontrollers/silicon.html#rp2040)
in [Lua](https://www.lua.org/)**. It packages the latest Lua interpreter with
bindings for the [Pico SDK](https://github.com/raspberrypi/pico-sdk) and a
cooperative threading library.

MicroLua is licensed under the [MIT](LICENSE.txt) license.

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

But to be honest, it's just an excuse for me to play with Lua and the RP2040,
and see how far I can push them.

## Features

- **Pristine, unpatched Lua interpreter:** MicroLua runs the latest,
  unmodified Lua interpreter, imported as a git submodule. All customization is
  done through [`luaconf.h`](core/luaconf.in.h).
- **Per-core interpreter instances:** MicroLua runs a separate Lua interpreter
  in each core. They don't share state except through C libraries.
- **Cooperative multithreading through Lua coroutines:** MicroLua implements
  cooperative threads as coroutines. This enables multitasking without the need
  for locks. Blocking library calls (e.g. `mlua.time.sleep_for()`) yield to
  other threads.
- **Thin bindings to C libraries:** MicroLua exposes a growing subset of the
  functionality provided by the Pico SDK. The bindings are designed with a
  straightforward and consistent [mapping](docs/core.md#binding-conventions) to
  their underlying C implementation.
- **Support for [Fennel](https://fennel-lang.org/):** Fennel sources are
  transpiled to Lua.
- **Comprehensive suite of unit tests:** They not only test the binding layer,
  but when possible also the underlying functionality of the Pico SDK.

### Performance

Performance is adequate for applications that don't require very low latency.
Event dispatch latency is on the order of 20 microseconds when running from
flash at 250 MHz. So it probably isn't realistic to try bit-banging high-speed
serial protocols or PWM in Lua, but that's what the PIO and PWM peripherals are
for. Anything that requires precise timings should probably be implemented in C.
But Lua is a great glue language for the non timing-critical logic, and very
easy to interface to C code.

### Roadmap

- **Add more bindings for the Pico SDK.** Next on the list are USB and
  Bluetooth. Eventually, most SDK libraries will have a binding.
- **Improve cross-core communication.** Each core runs its own Lua interpreter,
  so they cannot communicate directly through shared Lua state. Currently, the
  only way for the cores to communicate is the SIO FIFOs, which is fairly
  limited. A form of memory-based cross-core channel would be useful.
- **Add multi-chip communication.** As an extension of cross-core channels,
  cross-chip channels could enable fast communication between multiple RP2040
  chips.

## Examples

The [MicroLua-examples](https://github.com/MicroLua/MicroLua-examples)
repository contains example programs that demonstrate how to use the features of
the RP2040 from Lua.

Here's the `blink` example in MicroLua, a direct translation of the
[`blink`](https://github.com/raspberrypi/pico-examples/tree/master/blink)
example from the [`pico-examples`](https://github.com/raspberrypi/pico-examples)
repository.

```lua
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

## Documentation

- [Core functionality](docs/core.md) of MicroLua.
- [`mlua.*`](docs/mlua.md): MicroLua libraries.
- [`hardware.*`](docs/hardware.md): Bindings for the `hardware_*` libraries of
  the Pico SDK.
- [`pico.*`](docs/pico.md): Bindings for the `pico_*` libraries of the Pico SDK.
- [`lwip.*`](docs/lwip.md): Bindings for the lwIP library.

## Test suite

Here's how to build and run the test suite.

```shell
# Clone the repository and initialize submodules.
$ git clone https://github.com/MicroLua/MicroLua.git
$ cd MicroLua
$ git submodule update --init
$ git -C ext/pico-sdk submodule update --init

# Build the test suite for the host and run it.
$ tools/run -l -t bin/mlua_tests -p host

# Build the test suite for a "pico" board, flash it with picotool and connect
# to its virtual serial port with socat to view the test results. The target
# should be in BOOTSEL mode.
$ tools/run -l -t bin/mlua_tests -p pico -c -DPICO_BOARD=pico
```

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

### Does MicroLua support other microcontroller families?

Currently it doesn't, but it shouldn't be too difficult to add. The build system
is already multi-platform, as it can create binaries for the RP2040 and for the
build host. However, adding support for more platforms isn't a high priority.

MicroLua will likely support later RP devices if / when they get released.

### How does MicroLua compare to other Lua projects for the Pico?

- [picolua](https://github.com/kevinboone/luapico) is based on a patched Lua 5.4
  interpreter, and aims to provide a full embedded development environment,
  including a shell and an editor. It exposes a limited subset of Pico-specific functionality.

  MicroLua uses an unpatched Lua interpreter at the latest version, and aims to
  expose most of the functionality provided by the Pico SDK through a thin
  binding layer.

### What's the relationship with MicroLua DS?

[MicroLua DS](https://sourceforge.net/projects/microlua/) was a development
environment for building apps for the Nintendo DS in Lua. It was last released
in January 2014. MicroLua has no relationship with MicroLua DS.

While the naming conflict is unfortunate, I felt that almost 10 years of
inactivity was long enough that it was fair game to re-use the name.
