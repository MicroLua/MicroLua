# `mlua.*` modules

<!-- Copyright 2023 Remy Blank <remy@c-space.org> -->
<!-- SPDX-License-Identifier: MIT -->

This page describes the modules specific to MicroLua.

The test modules can be useful as usage examples.

## Globals

- `_VERSION: string = LUA_VERSION`\
  `_RELEASE: string = LUA_RELEASE`

- `WeakK: table = {__mode = 'k'}`\
  A metatable for weak keys.

- `arg: table`\
  The arguments passed to `main()`. The table is zero-based: `arg[0]` contains
  the executable name, and the following indexes contain the command-line
  arguments. The global isn't set when running on a target that doesn't have
  command-line arguments.

- `module(name)`\
  Create a new empty module.

- `try(fn, [arg, ...]) -> (results) | (fail, err)`\
  Call `fn` with the given arguments, and return the results of the call. If the
  call raises an error, return `fail` and the error that was raised. This
  function is kind of the opposite of `assert()`: it transforms errors into
  `(fail, err)`, whereas `assert()` transforms `(fail, err)` into errors.

  `try()` is slightly more convenient than `pcall()` when the first result is
  known to not be `nil`, as it doesn't insert the status code. For example, to
  require a module that may not be present:

  ```lua
  local i2c = try(require, 'hardware.i2c')
  if i2c then ... end
  ```

## `mlua.block`

**Module:** [`mlua.block`](../lib/common/mlua.block.c),
build target: `mlua_mod_mlua.block`

This module provides an abstraction for defining block devices in C
(`mlua.block.Dev`). Functions that fail return `fail`, an error message and an
error code from [`mlua.errors`](#mluaerrors).

- `Dev:read(offset, size) -> string | (fail, msg, err)`\
  Read from the block device. `offset` and `size` must be multiples of
  `read_size`.

- `Dev:write(offset, data) -> true | (fail, msg, err)`\
  Write to the block device. `offset` and `size` must be multiples of
  `write_size`.

- `Dev:erase(offset, size) -> true | (fail, msg, err)`\
  Erase a range of the block device. `offset` and `size` must be multiples of
  `erase_size`.

- `Dev:sync() -> true | (fail, msg, err)`\
  Flush all writes to the block device.

- `Dev:size() -> (integer, integer, integer, integer)`\
  Return the size of the block device in bytes, as well as `read_size`,
  `write_size` and `erase_size`.

## `mlua.block.flash`

**Module:** [`mlua.block.flash`](../lib/pico/mlua.block.flash.c),
build target: `mlua_mod_mlua.block.flash`

This module provides a block device that uses the QSPI flash for storage.

- `new(offset, size) -> Dev`\
  Create a new flash block device starting at `offset` in flash memory. `offset`
  is rounded up to the next multiple of `FLASH_SECTOR_SIZE`. `size` is adjusted so that `offset + size` is rounded down to the previous multiple of
  `FLASH_SECTOR_SIZE`.

## `mlua.block.mem`

**Module:** [`mlua.block.mem`](../lib/common/mlua.block.mem.c),
build target: `mlua_mod_mlua.block.mem`

This module provides a block device that uses a `mlua.mem.Buffer` for storage,
i.e. a contiguous block of RAM.

- `new(buffer, size, write_size = 256, erase_size = 256) -> Dev`\
  Create a new memory block device in `buffer`.

## `mlua.config`

**Module:** `mlua.config` (auto-generated),
tests: [`mlua.config.test`](../lib/common/mlua.config.test.lua)

This module is auto-generated for all executables defined with
`mlua_add_executable()`, and contains the symbols defined by
`mlua_target_config()` calls. Symbol definitions have the format
`{name}:{type}={value}`, where `{type}` is one of `boolean`, `integer`, `number`
or `string`.

**Example:**

```cmake
mlua_add_executable(example_executable)
mlua_target_config(example_executable
    my_bool:boolean=true
    my_int:integer=42
    my_num:number=123.456
    my_str:string=\"some-string\"
)
```

## `mlua.errors`

**Module:** [`mlua.errors`](../lib/common/mlua.errors.c),
build target: `mlua_mod_mlua.errors`,
tests: [`mlua.errors.test`](../lib/common/mlua.errors.test.lua)

This module provides a common set of error codes, for use by other modules. Note
that although the error codes look very much like `errno` values, they have
nothing to do with `errno` and have different numeric values.

- `message(code) -> string`\
  Return a string describing the given error code.

## `mlua.fs`

**Module:** [`mlua.fs`](../lib/common/mlua.fs.c),
build target: `mlua_mod_mlua.fs`,
tests: [`mlua.fs.test`](../lib/common/mlua.fs.test.lua)

This module provides functionality that is common across all filesystems.

- `TYPE_REG: integer`\
  `TYPE_DIR: integer`\
  File types: regular file and directory.

- `O_RDONLY: integer`\
  `O_WRONLY: integer`\
  `O_RDWR: integer`\
  `O_CREAT: integer`\
  `O_EXCL: integer`\
  `O_TRUNC: integer`\
  `O_APPEND: integer`\
  Flags that can be passed when opening a file.

- `SEEK_SET: integer`\
  `SEEK_CUR: integer`\
  `SEEK_END: integer`\
  Values that can be passed when seeking in a file.

- `join(path, ...) -> string`\
  Join pathnames. Ignores previous parts if a part is absolute. Inserts a `/`
  unless the first part is empty or already ends in `/`.

- `split(path) -> (string, string)`\
  Split a path into containing directory and basename.

## `mlua.fs.loader`

**Module:** [`mlua.fs.loader`](../lib/pico/mlua.fs.loader.c),
build target: `mlua_mod_mlua.fs.loader`,
tests: [`mlua.fs.loader.test`](../lib/pico/mlua.fs.loader.test.lua)

This module creates a global filesystem early in the boot process, and registers
a module searcher that looks up modules in that filesystem. When the module is
linked-in, it is auto-loaded during interpreter creation, which enables loading
the main module from the filesystem, for both cores.

The filesystem is mounted as littlefs ([`mlua.fs.lfs`](#mluafslfs)) and uses the
QSPI flash for storage ([`mlua.block.flash`](#mluablockflash)). It can be
customized with the following compile definitions:

- `MLUA_FS_LOADER_OFFSET` (default: `PICO_FLASH_SIZE_BYTES -
  MLUA_FS_LOADER_SIZE`): The offset in flash memory where the filesystem starts.
  Must be aligned on a `FLASH_SECTOR_SIZE` boundary. The default value places it
  at the end of the flash memory.
- `MLUA_FS_LOADER_SIZE` (default: 1 MiB): The size of the filesystem. Must be a
  multiple of `FLASH_SECTOR_SIZE`.
- `MLUA_FS_LOADER_BASE` (default: `"/lua"`): The path below which to look for
  modules.
- `MLUA_FS_LOADER_FLAT` (default: 1): When true, look up modules in the
  directory `MLUA_FS_LOADER_BASE`, i.e. the module `a.b.c` is loaded from the
  file `/lua/a.b.c.lua`. When false, look up modules in `MLUA_FS_LOADER_BASE`
  and its subdirectories, i.e. the module `a.b.c` is loaded either from
  `/lua/a/b/c.lua` or `/lua/a/b/c/init.lua`.

Information about the filesystem (flash range, type) is available as binary
info, and can be viewed with `picotool info -a`.

- `block: mlua.block.Dev`\
  The block device on which the filesystem operates.

- `fs: mlua.fs.lfs.Filesystem`\
  The filesystem from which modules are loaded.

## `mlua.fs.lfs`

**Module:** [`mlua.fs.lfs`](../lib/common/mlua.fs.lfs.c),
build target: `mlua_mod_mlua.fs.lfs`,
tests: [`mlua.fs.lfs.test`](../lib/common/mlua.fs.lfs.test.lua)

This module provides bindings for the
[littlefs](https://github.com/littlefs-project/littlefs) library. It allows
creating and mounting filesystems on [block devices](#mluablock) and operating
on their content. The following compile definitions affect how the bindings are
exposed:

- `LFS_READONLY`: When defined, allow only read operations.
- `LFS_MIGRATE`: When defined, provide the `Filesystem:migrate()` function.
- `LFS_THREADSAFE`: When defined, enables locking on filesystems, allowing
  access from both cores.

Functions that fail return `fail`, an error message and an error code from
[`mlua.errors`](#mluaerrors).

- `VERSION: integer`\
  `DISK_VERSION: integer`\
  The library and disk format versions.

- `NAME_MAX: integer`\
  `FILE_MAX: integer`\
  `ATTR_MAX: integer`\
  The maximum size of file names, file content and custom attributes.

- `new(device) -> Filesystem`\
  Create a filesystem object operating on the given block device. This doesn't
  format or mount the filesystem; it only binds a filesystem to a device.

### `Filesystem`

The `Filesystem` type (`mlua.fs.lfs.Filesystem`) represents a filesystem.

- `Filesystem:format(size) -> true | (fail, msg, err)`\
  Format the underlying block device for a filesystem of the given size. If
  `size` is missing, the filesystem fills the whole block device. The filesystem
  must not be mounted.

- `Filesystem:mount() -> true | (fail, msg, err)`\
  Mount an existing filesystem.

- `Filesystem:unmount() -> true | (fail, msg, err)`\
  `Filesystem:__close() -> true | (fail, msg, err)`\
  Unmount the filesystem.

- `Filesystem:is_mounted() -> bool`\
  Return `true` iff the filesystem is mounted.

- `Filesystem:grow(size) -> true | (fail, msg, err)`\
  Grow a mounted filesystem to the given size. If `size` is missing, the
  filesystem is grown to fill the whole block device.

- `Filesystem:statvfs() -> (6 * integer) | (fail, msg, err)`\
  Return information about the filesystem, as described by `struct lfs_fsinfo`:
  the on-disk version (`disk_version`), the size of a logical block
  (`block_size`), the number of logical blocks (`block_count`), and the limits
  on file names (`name_max`), file content (`file_max`) and custom attributes
  (`attr_max`).

- `Filesystem:size() -> integer | (fail, msg, err)`\
  Return the number of allocated blocks in the filesystem.

- `Filesystem:gc() -> true | (fail, msg, err)`\
  Attempt to proactively find free blocks.

- `Filesystem:traverse(callback) -> true | (fail, msg, err)`\
  Call `callback` with each block address that is currently in use.

- `Filesystem:mkconsistent() -> true | (fail, msg, err)`\
  Attempt to make the filesystem consistent and ready for writing.

- `Filesystem:migrate(size) -> true | (fail, msg, err)`\
  Attempt to migrate a previous version of littlefs. If `size` is missing, the
  filesystem fills the whole block device. The filesystem must not be mounted.
  Only defined if `LFS_MIGRATE` is defined.

- `Filesystem:open(path, flags) -> File | (fail, msg, err)`\
  Open a file. `flags` is a bitwise-or of `fs.O_*` values.

- `Filesystem:list(path) -> iter(name, type, [size]) | (fail, msg, err)`\
  List the content of a directory. Returns an iterator yielding the directory
  entries in arbitrary order. `size` is only returned for regular files.

- `Filesystem:stat(path) -> (name, type, [size]) | (fail, msg, err)`\
  Return information about a file. `size` is only returned for regular files.

- `Filesystem:getattr(path, attr) -> string | (fail, msg, err)`\
  `Filesystem:setattr(path, attr, value) -> true | (fail, msg, err)`\
  `Filesystem:removeattr(path, attr) -> true | (fail, msg, err)`\
  Get, set or remove a custom attribute from a file.

- `Filesystem:mkdir(path) -> true | (fail, msg, err)`\
  Create a directory.

- `Filesystem:remove(path) -> true | (fail, msg, err)`\
  Remove a file.

- `Filesystem:rename(old_path, new_path) -> true | (fail, msg, err)`\
  Rename a file.

### `File`

The `File` type (`mlua.fs.lfs.File`) represents an open file.

- `File:close() -> true | (fail, msg, err)`\
  `File:__close() -> true | (fail, msg, err)`\
  `File:__gc() -> true | (fail, msg, err)`\
  Close the file.

- `File:sync() -> true | (fail, msg, err)`\
  Synchronize the file to storage.

- `File:read(size) -> string | (fail, msg, err)`\
  Read data from the file.

- `File:write(data) -> integer | (fail, msg, err)`\
  Write data to the file. Returns the number of bytes written.

- `File:seek(offset, whence = SEEK_SET) -> integer | (fail, msg, err)`\
  Change the current position in the file. Returns the new position from the
  start of the file.

- `File:rewind() -> integer | (fail, msg, err)`\
  Change the current position to the start of the file (equivalent to
  `seek(0, SEEK_SET)`). Returns the new position from the start of the file (0).

- `File:tell() -> integer | (fail, msg, err)`\
  Return the current position in the file (equivalent to `seek(0, SEEK_CUR)`).

- `File:size() -> integer | (fail, msg, err)`\
  Return the size of the file.

- `File:truncate(size) -> true | (fail, msg, err)`\
  Truncate the file at the given size.

## `mlua.int64`

**Module:** [`mlua.int64`](../lib/common/mlua.int64.c),
build target: `mlua_mod_mlua.int64`,
tests: [`mlua.int64.test`](../lib/common/mlua.int64.test.lua)

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

**Module:** [`mlua.io`](../lib/common/mlua.io.lua),
build target: `mlua_mod_mlua.io`,
tests: [`mlua.io.test`](../lib/common/mlua.io.test.lua)

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

**Module:** [`mlua.list`](../lib/common/mlua.list.c),
build target: `mlua_mod_mlua.list`,
tests: [`mlua.list.test`](../lib/common/mlua.list.test.lua)

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

**Module:** [`mlua.mem`](../lib/common/mlua.mem.c),
build target: `mlua_mod_mlua.mem`,
tests: [`mlua.mem.test`](../lib/common/mlua.mem.test.lua)

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

- `Buffer:fill(value = 0, offset = 0, len = size - offset)`\
  Fill `len` bytes of the buffer, starting at `offset`, with `value`.

- `Buffer:read(offset = 0, len = size - offset) -> string`\
  Read `len` bytes from the buffer, starting at `offset`.

- `Buffer:write(data, offset = 0)`\
  Write `data` to the buffer, starting at `offset`.

## `mlua.oo`

**Module:** [`mlua.oo`](../lib/common/mlua.oo.lua),
build target: `mlua_mod_mlua.oo`,
tests: [`mlua.oo.test`](../lib/common/mlua.oo.test.lua)

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

## `mlua.platform`

**Module:** [`mlua.platform`](../lib/common/mlua.platform.c),
build target: `mlua_mod_mlua.platform`,
tests: [`mlua.platform.test`](../lib/common/mlua.platform.test.lua)

This module exposes platform-specific functionality under a common interface.

- `name: string`\
  The name of the platform for which the binary was built (`MLUA_PLATFORM`).

- `flash: table | false`\
  A description of the flash memory provided by the platform, or `false` if the
  platform doesn't have any flash memory. The table has the fields `address`,
  `size`, `write_size` and `erase_size`.

- `binary_size: integer | Int64`\
  The size of the binary, or zero if the size is unknown.

## `mlua.stdio`

**Module:** [`mlua.stdio`](../lib/common/mlua.stdio.c),
build target: `mlua_mod_mlua.stdio`,
tests: [`mlua.stdio.test`](../lib/common/mlua.stdio.test.lua)

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

**Module:** [`mlua.testing`](../lib/common/mlua.testing.lua),
build target: `mlua_mod_mlua.testing`,
tests: [`mlua.testing.test`](../lib/common/mlua.testing.test.lua)

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

- `Matcher:matches(want) -> self`\
  Declare an expectation that the value is a string that matches the pattern
  `want`.

- `Matcher:has(key) -> self`\
  `Matcher:not_has(key) -> self`\
  Declare an expectation that the value has or doesn't have the key `key`.

- `Matcher:raises([want])`\
  Declare an expectations that calling the value raises an error whose value
  matches the string pattern `want`.

### Test helper modules

- [`mlua.testing.clocks`](../lib/pico/mlua.testing.clocks.lua): Helpers for
  testing clock-related functionality.
- [`mlua.testing.i2c`](../lib/pico/mlua.testing.i2c.lua): Helpers for testing
  I2C functionality.
- [`mlua.testing.stdio`](../lib/pico/mlua.testing.stdio.lua): Helpers for
  testing stdio functionality.
- [`mlua.testing.uart`](../lib/pico/mlua.testing.uart.lua): Helpers for testing
  UART functionality.

## `mlua.thread`

**Module:** [`mlua.thread`](../lib/common/mlua.thread.lua),
build target: `mlua_mod_mlua.thread`,
tests: [`mlua.thread.test`](../lib/common/mlua.thread.test.lua)

This module provides cooperative threading functionality based on coroutines.
It sets the metaclass of the `coroutine` type to `Thread`, so coroutines are
effectively threads, and `Thread` methods can be called on coroutines. Thread
scheduling is based on an active queue and a wait list. Threads on the active
queue are run round-robin until they yield or terminate. Threads on the wait
list are resumed either explicitly or due to their deadline expiring.

When this module is linked in, the interpreter setup code creates a new thread
to run the configured main function, then runs `main()`.

- `start(fn, [name]) -> Thread`\
  Start a new thread that runs `fn()`, optionally giving it a name.

- `shutdown(result)`\
  Shut down the thread scheduler, and return `result` from `main()`. This
  function yields and therefore never returns. During shutdown, all threads are
  killed and their resources are freed.

- `yield()`\
  Yield from the running thread. The thread remains in the active queue, and is
  resumed after other active threads have run and yielded.

- `suspend([deadline])`\
  Suspend the running thread. If a deadline is provided, the thread is resumed
  at that time. Otherwise, it is suspended indefinitely.

- `running() -> Thread`\
  Return the currently-running thread.

- `blocking([enable]) -> boolean`\
  When `enable` is `true`, request blocking event processing for the running
  thread (i.e. don't yield to other threads while waiting for events). When
  `enable` is `false`, request non-blocking event processing. If `enable` isn't
  provided, don't modify the flag. Returns the previous value of the flag.

  The "blocking" flag is inherited from the running thread when starting a new
  thread.

- `main()`\
  Run the thread scheduler loop.

### `Thread`

This type represents an independent thread of execution. Threads are implemented
as coroutines, so they have to yield explicitly to allow other threads to run.
Many blocking library functions can yield when they have to wait, and their
documentation mentions it explicitly.

- `Thread.start(fn, [name]) -> Thread`\
  Start a new thread that runs `fn()`, optionally giving it a name.

- `Thread.shutdown(result)`\
  Shut down the thread scheduler, and return `result` from `main()`. This
  function yields and therefore never returns. During shutdown, all threads are
  killed and their resources are freed.

- `Thread:name() -> string`\
  Return the name of the thread, or a generated name if none was set.

- `Thread:is_alive() -> boolean`\
  Return true iff the thread is alive, i.e. the status of its coroutine isn't
  "dead".

- `Thread:is_waiting() -> boolean`\
  Return true iff the thread is waiting to be resumed, either by a call to
  `resume()` or by a deadline given to `yield()`.

- `Thread:resume() -> boolean`\
  Resume the thread if it is on the wait list. Returns true iff the thread was
  on the wait list.

- `Thread:kill() -> boolean`\
  Kill the thread, and return `true` iff the thread was alive. This causes the
  coroutine to unwind the stack and close all to-be-closed variables, then
  resumes any other threads waiting in a call to `join()`.

- `Thread:join()`\
  `Thread:__close()`\
  Wait for the thread to terminate. If the thread terminates with an error, the
  function re-raises the error. If the thread is assigned to a to-be-closed
  variable, it is joined when the variable is closed.

## `mlua.thread.group`

**Module:** [`mlua.thread.group`](../lib/common/mlua.thread.group.lua),
build target: `mlua_mod_mlua.thread.group`

This module provides functionality to facilitate thread management. The symbols
exported by this module are automatically added to `mlua.thread`.

### `Group`

This type represents a set of threads that have a common lifetime. It allows
adding threads to the group and waiting for them to terminate. Threads that
terminate automatically get removed from the group.

- `Group() -> Group`\
  Create a new thread group.

- `Group:start(fn, [name]) -> Thread`\
  Start a new thread that runs `fn()` and add it to the group.

- `Group:join()`\
  `Group:__close()`\
  Join the threads in the group. If the group is assigned to a to-be-closed
  variable, it is joined when the variable is closed.

## `mlua.time`

**Module:** [`mlua.time`](../lib/common/mlua.time.c),
build target: `mlua_mod_mlua.time`,
tests: [`mlua.time.test`](../lib/common/mlua.time.test.lua)

This module provides platform-independent time functionality.

- `ticks_per_second: integer = 1e6`\
  The number of ticks counted per second.

- `ticks_min: Int64`\
  `ticks_max: Int64`\
  The range of values that can be returned by `ticks()`.

- `ticks() -> Int64`\
  Return the current absolute time in microsecond ticks, as given by a monotonic
  clock.

- `sleep_until(time)` *[yields]*\
  Suspend the current thread until the given absolute time (as returned by
  `ticks()`).

- `sleep_for(ticks)` *[yields]*\
  Suspend the current thread for the given number of microsecond ticks.

## `mlua.uf2`

**Module:** [`mlua.uf2`](../lib/common/mlua.uf2.lua),
build target: `mlua_mod_mlua.uf2`

This module provides helpers to parse and generate
[UF2 files](https://github.com/microsoft/uf2).

- `flag_noflash: integer`\
  `flag_file_container: integer`\
  `flag_family_id_present: integer`\
  `flag_md5_present: integer`\
  `flag_ext_present: integer`\
  Bit masks for the `flags` field.

- `family_id_rp2040: integer`\
  Family ID values.

- `block_size: integer`\
  The size of an UF2 block.

- `parse(block, start = 1) -> table`\
  Parse an UF2 block starting at `start` in `block`. Returns a table with the
  fields `flags`, `target_addr`, `payload_size`, `block_no`, `num_blocks`,
  `reserved` and `data`.

- `serialize(block) -> string`\
  Serialize an UF2 block. `block` is a table that must contain at least
  `target_addr`, `block_no`, `num_blocks` and `data`, and optionally `flags`
  and `reserved` (default: 0).

## `mlua.util`

**Module:** [`mlua.util`](../lib/common/mlua.util.lua),
build target: `mlua_mod_mlua.util`,
tests: [`mlua.util.test`](../lib/common/mlua.util.test.lua)

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

- `raise(format, ...)`\
  Format an error message and raise an error with it. The error doesn't include
  location information.

- `check(...) -> ...`\
  Identical to `assert()`, but raised errors don't include location information.

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

- `percentile(values, p) -> number`\
  Compute the `p`th percentile of a list of values. `values` must be sorted.
