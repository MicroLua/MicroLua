# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

mlua_add_lua_modules(mlua_test_mlua mlua.test.lua)
target_link_libraries(mlua_test_mlua INTERFACE
    mlua_mod_mlua_io
    mlua_mod_mlua_mem
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua_block mlua.block.c)
target_include_directories(mlua_mod_mlua_block_headers INTERFACE
    include_mlua.block)
target_link_libraries(mlua_mod_mlua_block INTERFACE
    mlua_mod_mlua_errors
    mlua_mod_mlua_int64
)

mlua_add_c_module(mlua_mod_mlua_block_mem mlua.block.mem.c)
target_link_libraries(mlua_mod_mlua_block_mem INTERFACE
    mlua_mod_mlua_block
    mlua_mod_mlua_errors
    mlua_mod_mlua_mem
)

mlua_add_lua_modules(mlua_mod_mlua_cli mlua.cli.lua)
target_link_libraries(mlua_mod_mlua_cli INTERFACE
    mlua_mod_debug
    mlua_mod_math
    mlua_mod_mlua_io
    mlua_mod_mlua_list
    mlua_mod_mlua_util
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_config mlua.config.test.lua)

mlua_add_c_module(mlua_mod_mlua_errors mlua.errors.c)
target_include_directories(mlua_mod_mlua_errors_headers INTERFACE
    include_mlua.errors)

mlua_add_lua_modules(mlua_test_mlua_errors mlua.errors.test.lua)
target_link_libraries(mlua_test_mlua_errors INTERFACE
    mlua_mod_mlua_errors
)

mlua_add_c_module(mlua_mod_mlua_event mlua.event.c)
target_include_directories(mlua_mod_mlua_event_headers INTERFACE
    include_mlua.event)
target_link_libraries(mlua_mod_mlua_event INTERFACE
    mlua_mod_mlua_int64
    mlua_platform
)

mlua_add_c_module(mlua_mod_mlua_fs mlua.fs.c)
target_include_directories(mlua_mod_mlua_fs_headers INTERFACE
    include_mlua.fs)

mlua_add_lua_modules(mlua_test_mlua_fs mlua.fs.test.lua)
target_link_libraries(mlua_test_mlua_fs INTERFACE
    mlua_mod_mlua_fs
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua_fs_lfs mlua.fs.lfs.c)
target_include_directories(mlua_mod_mlua_fs_lfs_headers INTERFACE
    "${MLUA_LITTLEFS_SOURCE_DIR}"
    include_mlua.fs.lfs
)
target_compile_definitions(mlua_mod_mlua_fs_lfs INTERFACE
    LFS_NO_ASSERT=1
    LFS_NO_DEBUG=1
    LFS_NO_ERROR=1
    LFS_NO_MALLOC=1
    LFS_NO_WARN=1
)
target_sources(mlua_mod_mlua_fs_lfs INTERFACE
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs.c"
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs_util.c"
)
target_link_libraries(mlua_mod_mlua_fs_lfs INTERFACE
    mlua_mod_mlua_block
    mlua_mod_mlua_errors
    mlua_mod_mlua_fs_headers
)

mlua_add_lua_modules(mlua_test_mlua_fs_lfs mlua.fs.lfs.test.lua)
target_link_libraries(mlua_test_mlua_fs_lfs INTERFACE
    mlua_mod_mlua_block_mem
    mlua_mod_mlua_errors
    mlua_mod_mlua_fs
    mlua_mod_mlua_fs_lfs
    mlua_mod_mlua_list
    mlua_mod_mlua_mem
    mlua_mod_mlua_util
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua_int64 mlua.int64.c)
target_include_directories(mlua_mod_mlua_int64_headers INTERFACE
    include_mlua.int64)

mlua_add_lua_modules(mlua_test_mlua_int64 mlua.int64.test.lua)
target_link_libraries(mlua_test_mlua_int64 INTERFACE
    mlua_mod_math
    mlua_mod_mlua_int64
    mlua_mod_mlua_util
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua_io mlua.io.lua)
target_link_libraries(mlua_mod_mlua_io INTERFACE
    mlua_mod_mlua_oo
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_io mlua.io.test.lua)
target_link_libraries(mlua_test_mlua_io INTERFACE
    mlua_mod_mlua_io
    mlua_mod_mlua_oo
)

mlua_add_c_module(mlua_mod_mlua_list mlua.list.c)

mlua_add_lua_modules(mlua_test_mlua_list mlua.list.test.lua)
target_link_libraries(mlua_test_mlua_list INTERFACE
    mlua_mod_mlua_list
    mlua_mod_mlua_util
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua_mem mlua.mem.c)
target_include_directories(mlua_mod_mlua_mem_headers INTERFACE
    include_mlua.mem)
target_link_libraries(mlua_mod_mlua_mem INTERFACE
    mlua_mod_mlua_int64
)

mlua_add_lua_modules(mlua_test_mlua_mem mlua.mem.test.lua)
target_link_libraries(mlua_test_mlua_mem INTERFACE
    mlua_mod_mlua_mem
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_mlua_oo mlua.oo.lua)

mlua_add_lua_modules(mlua_test_mlua_oo mlua.oo.test.lua)
target_link_libraries(mlua_test_mlua_oo INTERFACE
    mlua_mod_mlua_oo
    mlua_mod_string
)

add_library(mlua_platform_luaconf INTERFACE)
add_library(mlua_platform INTERFACE)
target_compile_definitions(mlua_platform INTERFACE
    MLUA_PLATFORM=${MLUA_PLATFORM}
)
target_include_directories(mlua_platform INTERFACE include_platform)

mlua_add_c_module(mlua_mod_mlua_platform mlua.platform.c)
target_link_libraries(mlua_mod_mlua_platform INTERFACE
    mlua_mod_mlua_int64
    mlua_platform
)

mlua_add_lua_modules(mlua_test_mlua_platform mlua.platform.test.lua)
target_link_libraries(mlua_test_mlua_platform INTERFACE
    mlua_mod_mlua_platform
)

mlua_add_lua_modules(mlua_mod_mlua_shell mlua.shell.lua)
target_link_libraries(mlua_mod_mlua_shell INTERFACE
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_mlua_stdio mlua.stdio.c)

mlua_add_lua_modules(mlua_mod_mlua_testing mlua.testing.lua)
target_link_libraries(mlua_mod_mlua_testing INTERFACE
    mlua_mod_debug
    mlua_mod_math
    mlua_mod_mlua_cli
    mlua_mod_mlua_io
    mlua_mod_mlua_list
    mlua_mod_mlua_oo
    mlua_mod_mlua_platform
    mlua_mod_mlua_time
    mlua_mod_mlua_util
    mlua_mod_package
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_testing mlua.testing.test.lua)
target_link_libraries(mlua_test_mlua_testing INTERFACE
    mlua_mod_mlua_list
    mlua_mod_mlua_util
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua_thread mlua.thread.lua)
target_link_libraries(mlua_mod_mlua_thread INTERFACE
    mlua_mod_coroutine
    mlua_mod_mlua_event
    mlua_mod_mlua_oo
    mlua_mod_mlua_time
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_thread mlua.thread.test.lua)
target_link_libraries(mlua_test_mlua_thread INTERFACE
    mlua_mod_mlua_int64
    mlua_mod_mlua_thread
    mlua_mod_mlua_time
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_mlua_time mlua.time.c)
target_link_libraries(mlua_mod_mlua_time INTERFACE
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    mlua_platform
)

mlua_add_lua_modules(mlua_test_mlua_time mlua.time.test.lua)
target_link_libraries(mlua_test_mlua_time INTERFACE
    mlua_mod_mlua_time
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua_uf2 mlua.uf2.lua)

mlua_add_lua_modules(mlua_mod_mlua_util mlua.util.lua)
target_link_libraries(mlua_mod_mlua_util INTERFACE
    mlua_mod_math
    mlua_mod_mlua_list
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_util mlua.util.test.lua)
target_link_libraries(mlua_test_mlua_util INTERFACE
    mlua_mod_mlua_util
    mlua_mod_table
)
