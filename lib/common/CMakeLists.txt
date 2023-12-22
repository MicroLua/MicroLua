# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

mlua_add_lua_test_modules(mlua_mod_mlua_test mlua.test.lua)
target_link_libraries(mlua_mod_mlua_test INTERFACE
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

mlua_add_lua_test_modules(mlua_mod_mlua_config_test mlua.config.test.lua)

mlua_add_c_module(mlua_mod_mlua_errors mlua.errors.c)
target_include_directories(mlua_mod_mlua_errors_headers INTERFACE
    include_mlua.errors)

mlua_add_lua_test_modules(mlua_mod_mlua_errors_test
    mlua.errors.test.lua)
target_link_libraries(mlua_mod_mlua_errors_test INTERFACE
    mlua_mod_mlua_errors
)

mlua_add_c_module(mlua_mod_mlua_fs mlua.fs.c)
target_include_directories(mlua_mod_mlua_fs_headers INTERFACE
    include_mlua.fs)

mlua_add_lua_test_modules(mlua_mod_mlua_fs_test mlua.fs.test.lua)
target_link_libraries(mlua_mod_mlua_fs_test INTERFACE
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

mlua_add_lua_test_modules(mlua_mod_mlua_fs_lfs_test
    mlua.fs.lfs.test.lua)
target_link_libraries(mlua_mod_mlua_fs_lfs_test INTERFACE
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

mlua_add_lua_test_modules(mlua_mod_mlua_int64_test mlua.int64.test.lua)
target_link_libraries(mlua_mod_mlua_int64_test INTERFACE
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

mlua_add_lua_test_modules(mlua_mod_mlua_io_test mlua.io.test.lua)
target_link_libraries(mlua_mod_mlua_io_test INTERFACE
    mlua_mod_mlua_io
    mlua_mod_mlua_oo
)

mlua_add_c_module(mlua_mod_mlua_list mlua.list.c)

mlua_add_lua_test_modules(mlua_mod_mlua_list_test mlua.list.test.lua)
target_link_libraries(mlua_mod_mlua_list_test INTERFACE
    mlua_mod_mlua_list
    mlua_mod_mlua_util
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua_mem mlua.mem.c)
target_include_directories(mlua_mod_mlua_mem_headers INTERFACE
    include_mlua.mem)

mlua_add_lua_test_modules(mlua_mod_mlua_mem_test
    mlua.mem.test.lua)
target_link_libraries(mlua_mod_mlua_mem_test INTERFACE
    mlua_mod_mlua_mem
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_mlua_oo mlua.oo.lua)

mlua_add_lua_test_modules(mlua_mod_mlua_oo_test mlua.oo.test.lua)
target_link_libraries(mlua_mod_mlua_oo_test INTERFACE
    mlua_mod_mlua_oo
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_mlua_shell mlua.shell.lua)
target_link_libraries(mlua_mod_mlua_shell INTERFACE
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_mlua_util mlua.util.lua)
target_link_libraries(mlua_mod_mlua_util INTERFACE
    mlua_mod_math
    mlua_mod_mlua_list
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_test_modules(mlua_mod_mlua_util_test mlua.util.test.lua)
target_link_libraries(mlua_mod_mlua_util_test INTERFACE
    mlua_mod_mlua_util
    mlua_mod_table
)
