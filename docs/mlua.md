# `mlua.*` modules

<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

This page describes the modules specific to MicroLua.

The test modules can be useful as usage examples.

## Globals

- `_VERSION: string = LUA_VERSION`\
  `_RELEASE: string = LUA_RELEASE`

- `mlua`\
  The `mlua` module is auto-loaded on startup and made available as a global.

## `mlua`

**Module:** [`mlua`](../lib/mlua.lua),
tests: [`mlua.test`](../lib/mlua.test.lua)

The `mlua` module is auto-loaded on startup and made available as a global. This
simplifies [module definitions](core.md#lua-modules).

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

## `mlua.errors`

**Module:** [`mlua.errors`](../lib/mlua.errors.c),
build target: `mlua_mod_mlua_errors`,
tests: [`mlua.errors.test`](../lib/mlua.errors.test.lua)

This module provides a common set of error codes, for use by other modules. Note
that although the error codes look very much like `errno` values, they have
nothing to do with `errno` and have different numeric values.

> [!IMPORTANT]
> The numeric values of the error codes are **not** part of the API and can
> change at any time. Always compare error codes to the symbols in this module
> in error handling code.

- `message(code) -> string`\
  Return a string describing the given error code.

## `mlua.event`

**Module:** [`mlua.event`](../lib/mlua.event.c),
build target: `mlua_mod_mlua_event`

This module provides the core event handling and dispatch functionality.

- `dispatch(time)`\
  Wait for events to fire or the given time to pass. This function is used by
  the thread scheduler.

## `mlua.int64`

**Module:** [`mlua.int64`](../lib/mlua.int64.c),
build target: `mlua_mod_mlua_int64`,
tests: [`mlua.int64.test`](../lib/mlua.int64.test.lua)

This module provides a 64-bit signed integer type (`mlua.Int64`). When
`lua_Integer` is a 32-bit integer, `Int64` is a full userdata with all the
relevant metamethods, supporting mixed-type operations with `integer` (but no
automatic promotion to `number`). When `lua_Integer` is a 64-bit integer,
`Int64` is an alias for `integer`.

> [!IMPORTANT]
> **Mixed-type equality comparisons (`==`, `~=`) involving `Int64` values do not
> work correctly**, because Lua only calls the `__eq` metamethod if both values
> are either tables or full userdata. Use `eq()` instead if the arguments may
> be primitive types.

- `int64(value) -> Int64 | nil`\
  Cast `value` to an `Int64`. `value` can have the following types:

  - `boolean`: `false -> 0`, `true -> 1`
  - `integer`: Sign-extends `value` to an `Int64`. When `lua_Integer` is a
    32-bit integer, an optional second argument provides the high-order 32 bits.
  - `number`: Fails if `value` cannot be represented exactly as an `Int64`.
  - `string`: Parse the value from a string. Accepts an optional `base` argument
    (default: 0). When the base is 0, it is inferred from the value prefix
    (`0x`, `0X`: 16, `0o`: 8, `0b`: 2, otherwise: 10).

- `max: Int64`\
  The maximum value that an `Int64` can hold (`2^63-1`)

- `min: Int64`\
  The minimum value that an `Int64` can hold (`-2^63`).

- `ashr(value, num) -> Int64`\
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
build target: `mlua_mod_mlua_io`,
tests: [`mlua.io.test`](../lib/mlua.io.test.lua)

This module provides helpers for input / output processing.

> [!IMPORTANT]
> Output functions block without yielding if the output buffer for `stdout` is
> full.

- `read(count) -> string | nil`\
  Read at least one and at most `count` characters from `stdin`. Uses
  [`pico.stdio`](pico.md#picostdio) if the module is available, or blocks
  without yielding if no data is available.

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

## `mlua.list`

**Module:** [`mlua.list`](../lib/mlua.list.c),
build target: `mlua_mod_mlua_list`,
tests: [`mlua.list.test`](../lib/mlua.list.test.lua)

This module provides functions to operate on lists that can contain `nil`
values. Such lists track the length of the list at index `[0]`. The module
itself is a metatable (`mlua.List`) that can be set on tables to expose the
functions as methods.

- `list(list) -> List`\
  Convert `list` to a `List` by setting its length at index `[0]` and its
  metatable. If `list` is nil or missing, return an empty `List`.

- `len(list) -> integer`\
  Return the number of elements in `list`.

- `eq(lhs, rhs) -> boolean`\
  Return true iff the elements of `lhs` and `rhs` compare pairwise equal.

- `ipairs(list) -> function, list, 0`\
  Return an iterator over the elements of `list`.

- `append(list, value, ...) -> List`\
  Append one or more values to `list`, and return the resulting list. `list` can
  be `nil`, in which case the function creates a new empty list before appending
  the values.

- `insert(list, pos = #list + 1, value) -> list`\
  Insert `value` at position `pos` in `list`, shifting the following elements
  up by one position.

- `remove(list, pos = #list) -> any`\
  Remove the element at position `pos` from `list`, shifting the following
  elements down by one position, and return the removed value.

- `pack(...) -> List`\
  Return a new `List` containing the given arguments.

- `unpack(list, i = 1, j = #list) -> ...`\
  Return the elements at positions `i` to `j` in `list`.

- `concat(list, sep = '', i = 1, j = #list) -> string`\
  Return the concatenation of the elements of `list` at positions `i` to `j`,
  separated by `sep`.

- `find(list, value, start = 1) -> integer`\
  Return the index of the first value in `list` that compares equal to `value`,
  starting at index `start`, or `nil` if no such value is found.

## `mlua.mem`

**Module:** [`mlua.mem`](../lib/mlua.mem.c),
build target: `mlua_mod_mlua_mem`,
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

The `Buffer` type (`mlua.Buffer`) holds a fixed-size memory buffer.

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
build target: `mlua_mod_mlua_oo`,
tests: [`mlua.oo.test`](../lib/mlua.oo.test.lua)

This module provides a simple object model for object-oriented programming.
Classes are created with `class`, providing a name and optionally a base class.
Instances are created by calling the class, optionally with arguments. The
latter are passed to the instance initializer `__init`, if it is defined.
Metamethods can be defined on classes, and have the expected effect. They are
copied from the base class to the subclass at class creation time. This is
necessary because Lua gets metamethods using a raw access.

- `class(name, base) -> Class`\
  Create a class with the given name, optionally inheriting from `base`.

- `issubclass(cls, base) -> boolean`\
  Return true iff `cls` is a subclass of `base`.

- `isinstance(obj, cls) -> boolean`\
  Return true iff `obj` is an instance of `cls`.

## `mlua.stdio`

**Module:** [`mlua.stdio`](../lib/mlua.stdio.c),
build target: `mlua_mod_mlua_stdio`,
tests: [`mlua.stdio.test`](../lib/mlua.stdio.test.lua)

This module manages stdio input and output. When linked in, it is automatically
loaded during interpreter startup. It defines input and output stream types, and
initializes the stdio libraries as defined in the compile-time configuration.

> [!IMPORTANT]
> **Output functions block without yielding** if the output buffer for the
> stream is full.

- `_G.stdin: InStream`\
  `_G.stdout: OutStream`\
  `_G.stderr: OutStream`\
  The standard input and output streams. They are set in globals when the module
  is loaded.

- `_G.print(...)`\
  Print the given arguments on `stdout`.

### `InStream`

The `InStream` type (`mlua.InStream`) represents an input stream.

- `read(count) -> string | nil`\
  Read at least one and at most `count` characters from the stream. Uses
  [`pico.stdio`](pico.md#picostdio) if the module is available, or blocks
  without yielding if no data is available.

### `OutStream`

The `OutStream` type (`mlua.OutStream`) represents an output stream.

- `write(data) -> integer | nil`\
  Write data to the stream, and return the number of characters written.

## `mlua.testing`

**Module:** [`mlua.testing`](../lib/mlua.testing.lua),
build target: `mlua_mod_mlua_testing`,
tests: [`mlua.testing.test`](../lib/mlua.testing.test.lua)

This module is a unit-testing library inspired by the Go
[`testing`](https://pkg.go.dev/testing) package. The best way to understand how
the library works is by looking at the test suite included with MicroLua.

- `main()`\
  This function can be configured as a main function to execute tests from all
  linked-in modules. Modules whose name ends with `.test` are considered test
  modules, and functions in those modules whose name starts with `test_`  are
  considered test cases.

<!-- TODO: Document ExprFactory and Expr -->

### `Test`

The `Test` class represents a single unit test.

- `Test.helper`\
  A value that can be assigned to a local variable of a test helper to skip it
  when determining the location of a test failure.

- `Test.expr: ExprFactory`\
  `Test.mexpr: ExprFactory`\
  An expression factory. `expr` uses only the first return value, while `mexpr`
  packs all return values into a [`List`](#mlualist).

- `Test:cleanup(fn)`\
  Registers the function `fn` to be called after the test completes. Cleanup
  functions are executed in the reverse order of registration.

- `Test:patch(tab, name, value)`\
  Set `tab[name]` to `value`, and restore the previous value at the end of the
  test.

- `Test:repr(v) -> function`\
  Return a function that calls `util.repr(v)` when called.

- `Test:func(name, args) -> function`\
  Return a function that returns a string representation of a function call when
  called.

- `Test:printf(format, ...)`\
  Format a string and print it as part of the test output.

- `Test:log(format, ...)`\
  Format and log a message, together with the location of the log statement.
  Arguments of type `function` are called, and the return value is used for
  formatting.

- `Test:skip(format, ...)`\
  Abort the test and mark it as skipped. Arguments of type `function` are
  called, and the return value is used for formatting.

- `Test:error(format, ...)`\
  Log a test failure. The current test continues to execute. Arguments of type
  `function` are called, and the return value is used for formatting.

- `Test:fatal(format, ...)`\
  Log a test failure. The current test is aborted. Arguments of type `function`
  are called, and the return value is used for formatting.

- `Test:expect(cond, format, ...)`\
  `Test:expect(value) -> Matcher`\
  Declare a test expectation. The first form logs a test failure if `cond` is
  false. Arguments of type `function` are called, and the return value is used
  for formatting. The second form returns a `Matcher` on which further
  expectations can be declared. The current test continues to execute regardless
  of the outcome.

- `Test:assert(cond, format, ...)`\
  `Test:assert(value) -> Matcher`\
  Declare a test expectation. The first form logs a test failure if `cond` is
  false. Arguments of type `function` are called, and the return value is used
  for formatting. The second form returns a `Matcher` on which further
  expectations can be declared. If the expectation fails, the current test is
  aborted.

- `Test:failed() -> boolean`\
  Return true iff the test has failed.

- `Test:run(name, fn)`\
  Run the function `fn` as a sub-test. A new `Test` instance is provided as an
  argument.

- `Test:enable_output()`\
  Normally, test output is inhibited until a failure is logged. This function
  enables test output even if no failure has been logged.

- `Test:run_module(name, pat = '^test_')`\
  Import the module `name` and run all functions whose name matches the string
  pattern `pat` as sub-tests. Unload the module at the end of the test. If the
  module contains a function named `set_up`, it is called before the first test
  function.

- `Test:run_modules(mod_pat = '%.test$', func_pat = '^test_')`\
  Run tests from all linked-in modules whose names match the string pattern
  `mod_pat` as sub-tests.

### `Matcher`

A `Matcher` instance holds a value and allows declaring expectations against
that value.

- `Matcher:label(format, ...) -> self`\
  Set the label as which the value should be reported in test failures.

- `Matcher:func(name, ...) -> self`\
  Set the label as which the value should be reported in test failures as a
  text representation of the function `name` called with the given arguments.

- `Matcher:op(op = '=') -> self`\
  Set a label for the operation that is used to check an expectation.

- `Matcher:apply(fn) -> self`\
  Apply the function `fn` to the value, and set it as the new value.

- `Matcher:eq(want, eq = mlua.util.eq) -> self`\
  `Matcher:neq(want, eq = mlua.util.eq) -> self`\
  `Matcher:lt(want) -> self`\
  `Matcher:lte(want) -> self`\
  `Matcher:gt(want) -> self`\
  `Matcher:gte(want) -> self`\
  Declare an expectation that the value is equal, not equal, less than, less
  than or equal, greater than, or greater than or equal to `want`. Equality and
  inequality comparisons accept an optional equality comparison function.

- `Matcher:close_to(want, eps) -> self`\
  Declare an expectation that the value is within `eps` of `want`.

- `Matcher:close_to_rel(want, fact) -> self`\
  Declare an expectation that the value is within `want * fact` of `want`.

- `Matcher:raises(want)`\
  Declare an expectations that calling the value raises an error whose value
  matches the string pattern `want`.

### Test helper modules

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
build target: `mlua_mod_mlua_thread`,
tests: [`mlua.thread.test`](../lib/mlua.thread.test.lua)

This module provides cooperative threading functionality based on coroutines.
It sets the metaclass of the `coroutine` type to `Thread`, so coroutines are
effectively threads, and `Thread` methods can be called on coroutines. Thread
scheduling is based on an active queue and a wait list. Threads on the active
queue are run round-robin until they yield or terminate. Threads on the wait
list are resumed either explicitly or due to their deadline expiring.

When this module is linked in, the interpreter setup code creates a new thread
to run the configured main function, then runs `main()`.

- `start(fn) -> Thread`\
  Start a new thread that runs `fn()`.

- `shutdown()`\
  Shut down the thread scheduler. This function yields and therefore never
  returns. During shutdown, all threads are killed and their resources are
  freed.

- `yield(time)`\
  Yield from the current thread. If `time` is `nil`, the thread remains in the
  active queue and is resumed after other active threads have run and yielded.
  Otherwise it is moved to the wait list. If `time` is `false`, the thread can
  only be resumed by a call to `resume()`. Otherwise, the thread is resumed when
  the given time has passed, or by a call to `resume()`.

- `running() -> Thread`\
  Return the currently-running thread.

- `now() -> Int64`\
  Return the current time in microseconds.

- `sleep_until(time)`\
  Suspend the current thread until `time` has passed.

- `sleep_us(duration)`\
  Suspend the current thread for the given duration in microseconds.

- `main()`\
  Run the thread scheduler loop.

### `Thread`

This type represents an independent thread of execution. Threads are implemented
as coroutines, so they have to yield explicitly to allow other threads to run.
Many blocking library functions can yield when they have to wait, and their
documentation mentions it explicitly.

- `Thread.start(fn) -> Thread`\
  Start a new thread that runs `fn()`.

- `Thread.shutdown()`\
  Shut down the thread scheduler. This function yields and therefore never
  returns. During shutdown, all threads are killed and their resources are
  freed.

- `Thread:name() -> string`\
  Return the name of the thread, or a generated name if none was set.

- `Thread:set_name(n) -> Thread`\
  Set the name of the thread.

- `Thread:is_alive() -> boolean`\
  Return true iff the thread is alive, i.e. the status of its coroutine isn't
  "dead".

- `Thread:is_waiting() -> boolean`\
  Return true iff the thread is waiting to be resumed, either by a call to
  `resume()` or by a deadline given to `yield()`.

- `Thread:resume() -> boolean`\
  Resume the thread if it is on the wait list. Returns true iff the thread was
  on the wait list.

- `Thread:kill()`\
  Kill the thread. This causes the coroutine to unwind the stack and close all
  to-be-closed variables, then resumes any other threads waiting in a call to
  `join()`.

- `Thread:join()`\
  `Thread:__close()`\
  Wait for the thread to terminate. If the thread terminates with an error, the
  function re-raises the error. If the thread is assigned to a to-be-closed
  variable, it is joined when the variable is closed.

### `Group`

This type represents a set of threads that have a common lifetime. It allows
adding threads to the group and waiting for them to terminate. Threads that
terminate automatically get removed from the group.

- `Group() -> Group`\
  Create a new thread group.

- `Group:start(fn) -> Thread`\
  Start a new thread that runs `fn()` and add it to the group.

- `Group:join()`\
  `Group:__close()`\
  Join the threads in the group. If the group is assigned to a to-be-closed
  variable, it is joined when the variable is closed.

## `mlua.util`

**Module:** [`mlua.util`](../lib/mlua.util.lua),
build target: `mlua_mod_mlua_util`,
tests: [`mlua.util.test`](../lib/mlua.util.test.lua)

This module provides various utilities.

- `ident(...) -> ...`\
  Return the arguments unchanged.

- `eq(a, b) -> boolean`\
  `neq(a, b) -> boolean`\
  `lt(a, b) -> boolean`\
  `lte(a, b) -> boolean`\
  `gt(a, b) -> boolean`\
  `gte(a, b) -> boolean`\
  Wrapper functions for binary comparisons.

- `get(tab, key) -> any`\
  Return `tab[key]`, or `nil` if the lookup raises an error.

- `repr(v) -> string`\
  Return a human-readable string representation of `v`. Calls the `__repr`
  metamethod of `v` if it is defined.

- `keys(tab, filter) -> list`\
  Return the keys of `tab`. If `filter` is provided, only the keys of entries
  where `filter(key, value)` returns true are included.

- `values(tab, filter) -> list`\
  Return the values of `tab`. If `filter` is provided, only the values of
  entries where `filter(key, value)` returns true are included.

- `sort(items, comp) -> items`\
  Sort `items` and return it.

- `table_eq(a, b) -> boolean`\
  Return true iff the `(key, value)` pairs of the given tables compare equal.

- `table_copy(tab) -> table`\
  Return a shallow copy of `tab`.

- `table_comp(keys) -> function(a, b)`\
  Return a comparison function comparing table pairs by the elements at the
  given keys.
