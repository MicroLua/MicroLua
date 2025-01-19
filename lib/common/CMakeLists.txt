# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

add_library(mlua_platform_luaconf INTERFACE)
mlua_add_library(mlua_platform INTERFACE)
target_compile_definitions(mlua_platform_headers INTERFACE
    MLUA_PLATFORM=${MLUA_PLATFORM}
)
target_include_directories(mlua_platform_headers INTERFACE include_platform)

mlua_add_lua_modules(mlua_test_mlua mlua.test.lua)
target_link_libraries(mlua_test_mlua INTERFACE
    mlua_mod_mlua.io
    mlua_mod_mlua.mem
    mlua_mod_mlua.thread
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.array mlua.array.c)
target_link_libraries(mlua_mod_mlua.array INTERFACE
    mlua_mod_mlua.int64
)

mlua_add_lua_modules(mlua_test_mlua.array mlua.array.test.lua)
target_link_libraries(mlua_test_mlua.array INTERFACE
    mlua_mod_math
    mlua_mod_mlua.array
    mlua_mod_mlua.int64
    mlua_mod_mlua.list
    mlua_mod_mlua.mem
    mlua_mod_mlua.repr
    mlua_mod_string
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.bits mlua.bits.c)
target_link_libraries(mlua_mod_mlua.bits INTERFACE
    mlua_mod_mlua.int64
)

mlua_add_lua_modules(mlua_test_mlua.bits mlua.bits.test.lua)
target_link_libraries(mlua_test_mlua.bits INTERFACE
    mlua_mod_mlua.bits
    mlua_mod_mlua.int64
    mlua_mod_string
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.block mlua.block.c)
target_include_directories(mlua_mod_mlua.block_headers INTERFACE
    include_mlua.block)
target_link_libraries(mlua_mod_mlua.block INTERFACE
    mlua_mod_mlua.errors
    mlua_mod_mlua.int64
)

mlua_add_c_module(mlua_mod_mlua.block.mem mlua.block.mem.c)
target_link_libraries(mlua_mod_mlua.block.mem INTERFACE
    mlua_mod_mlua.block
    mlua_mod_mlua.errors
)

mlua_add_lua_modules(mlua_mod_mlua.cli mlua.cli.lua)
target_link_libraries(mlua_mod_mlua.cli INTERFACE
    mlua_mod_debug
    mlua_mod_math
    mlua_mod_mlua.io
    mlua_mod_mlua.list
    mlua_mod_mlua.util
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua.config mlua.config.test.lua)

mlua_add_c_module(mlua_mod_mlua.errors mlua.errors.c)
target_include_directories(mlua_mod_mlua.errors_headers INTERFACE
    include_mlua.errors)

mlua_add_lua_modules(mlua_test_mlua.errors mlua.errors.test.lua)
target_link_libraries(mlua_test_mlua.errors INTERFACE
    mlua_mod_mlua.errors
)

mlua_add_c_module(mlua_mod_mlua.fs mlua.fs.c)
target_include_directories(mlua_mod_mlua.fs_headers INTERFACE
    include_mlua.fs)

mlua_add_lua_modules(mlua_test_mlua.fs mlua.fs.test.lua)
target_link_libraries(mlua_test_mlua.fs INTERFACE
    mlua_mod_mlua.fs
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.fs.lfs mlua.fs.lfs.c)
target_include_directories(mlua_mod_mlua.fs.lfs_headers INTERFACE
    "${MLUA_LITTLEFS_SOURCE_DIR}"
    include_mlua.fs.lfs
)
target_compile_definitions(mlua_mod_mlua.fs.lfs_headers INTERFACE
    LFS_NO_ASSERT=1
    LFS_NO_DEBUG=1
    LFS_NO_ERROR=1
    LFS_NO_MALLOC=1
    LFS_NO_WARN=1
)
target_sources(mlua_mod_mlua.fs.lfs INTERFACE
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs.c"
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs_util.c"
)
mlua_mirrored_target_link_libraries(mlua_mod_mlua.fs.lfs INTERFACE
    mlua_mod_mlua.block
)
target_link_libraries(mlua_mod_mlua.fs.lfs INTERFACE
    mlua_mod_mlua.errors
    mlua_mod_mlua.fs_headers
)

mlua_add_lua_modules(mlua_test_mlua.fs.lfs mlua.fs.lfs.test.lua)
target_link_libraries(mlua_test_mlua.fs.lfs INTERFACE
    mlua_mod_mlua.block.mem
    mlua_mod_mlua.errors
    mlua_mod_mlua.fs
    mlua_mod_mlua.fs.lfs
    mlua_mod_mlua.list
    mlua_mod_mlua.mem
    mlua_mod_mlua.util
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.int64 mlua.int64.c)
target_include_directories(mlua_mod_mlua.int64_headers INTERFACE
    include_mlua.int64)

mlua_add_lua_modules(mlua_test_mlua.int64 mlua.int64.test.lua)
target_link_libraries(mlua_test_mlua.int64 INTERFACE
    mlua_mod_math
    mlua_mod_mlua.int64
    mlua_mod_mlua.util
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua.io mlua.io.lua)
target_link_libraries(mlua_mod_mlua.io INTERFACE
    mlua_mod_mlua.oo
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua.io mlua.io.test.lua)
target_link_libraries(mlua_test_mlua.io INTERFACE
    mlua_mod_mlua.io
    mlua_mod_mlua.oo
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.list mlua.list.c)
target_link_libraries(mlua_mod_mlua.list INTERFACE
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua.list mlua.list.test.lua)
target_link_libraries(mlua_test_mlua.list INTERFACE
    mlua_mod_mlua.list
    mlua_mod_mlua.repr
    mlua_mod_mlua.util
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.mem mlua.mem.c)
target_link_libraries(mlua_mod_mlua.mem INTERFACE
    mlua_mod_mlua.int64
)

mlua_add_lua_modules(mlua_test_mlua.mem mlua.mem.test.lua)
target_link_libraries(mlua_test_mlua.mem INTERFACE
    mlua_mod_mlua.list
    mlua_mod_mlua.mem
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua.oo mlua.oo.lua)

mlua_add_lua_modules(mlua_test_mlua.oo mlua.oo.test.lua)
target_link_libraries(mlua_test_mlua.oo INTERFACE
    mlua_mod_mlua.oo
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_mlua.platform mlua.platform.c)
target_link_libraries(mlua_mod_mlua.platform INTERFACE
    mlua_mod_mlua.int64
)

mlua_add_lua_modules(mlua_test_mlua.platform mlua.platform.test.lua)
target_link_libraries(mlua_test_mlua.platform INTERFACE
    mlua_mod_mlua.platform
)

mlua_add_lua_modules(mlua_mod_mlua.repr mlua.repr.lua)
target_link_libraries(mlua_mod_mlua.repr INTERFACE
    mlua_mod_math
    mlua_mod_mlua.list
    mlua_mod_string
)

mlua_add_lua_modules(mlua_test_mlua.repr mlua.repr.test.lua)
target_link_libraries(mlua_test_mlua.repr INTERFACE
    mlua_mod_mlua.repr
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua.shell mlua.shell.lua)
target_link_libraries(mlua_mod_mlua.shell INTERFACE
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_mlua.stdio mlua.stdio.c)

mlua_add_lua_modules(mlua_mod_mlua.testing mlua.testing.lua)
target_link_libraries(mlua_mod_mlua.testing INTERFACE
    mlua_mod_debug
    mlua_mod_math
    mlua_mod_mlua.cli
    mlua_mod_mlua.io
    mlua_mod_mlua.list
    mlua_mod_mlua.oo
    mlua_mod_mlua.repr
    mlua_mod_mlua.time
    mlua_mod_mlua.util
    mlua_mod_package
    mlua_mod_string
)

mlua_add_lua_modules(mlua_test_mlua.testing mlua.testing.test.lua)
target_link_libraries(mlua_test_mlua.testing INTERFACE
    mlua_mod_mlua.list
    mlua_mod_mlua.repr
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.thread mlua.thread.c)
target_compile_definitions(mlua_mod_mlua.thread_headers INTERFACE
    MLUA_EXTRASPACE=16
)
target_include_directories(mlua_mod_mlua.thread_headers INTERFACE
    include_mlua.thread)
target_link_libraries(mlua_mod_mlua.thread INTERFACE
    mlua_mod_mlua.int64
)

mlua_add_lua_modules(mlua_test_mlua.thread mlua.thread.test.lua)
target_link_libraries(mlua_test_mlua.thread INTERFACE
    mlua_mod_coroutine
    mlua_mod_math
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread
    mlua_mod_mlua.thread.group
    mlua_mod_mlua.time
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_mlua.thread.group mlua.thread.group.lua)
target_link_libraries(mlua_mod_mlua.thread.group INTERFACE
    mlua_mod_mlua.oo
    mlua_mod_mlua.thread
)

mlua_add_c_module(mlua_mod_mlua.time mlua.time.c)
target_link_libraries(mlua_mod_mlua.time INTERFACE
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread_headers
)

mlua_add_lua_modules(mlua_test_mlua.time mlua.time.test.lua)
target_link_libraries(mlua_test_mlua.time INTERFACE
    mlua_mod_math
    mlua_mod_mlua.int64
    mlua_mod_mlua.time
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua.uf2 mlua.uf2.lua)

mlua_add_lua_modules(mlua_mod_mlua.util mlua.util.lua)
target_link_libraries(mlua_mod_mlua.util INTERFACE
    mlua_mod_math
    mlua_mod_mlua.list
    mlua_mod_string
)

mlua_add_lua_modules(mlua_test_mlua.util mlua.util.test.lua)
target_link_libraries(mlua_test_mlua.util INTERFACE
    mlua_mod_mlua.util
    mlua_mod_table
)
