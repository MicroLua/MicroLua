# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

project(tools C)
set(CMAKE_C_STANDARD 11)

include("${MLUA_PATH}/rules.cmake")

mlua_add_compile_options()
add_subdirectory("${MLUA_PATH}/core" core)
add_subdirectory("${MLUA_PATH}/lib/common" lib/common)

# Module: gen
mlua_add_lua_modules(gen_main NOCOMPILE gen.lua)
target_link_libraries(gen_main INTERFACE
    mlua_mod_io
    mlua_mod_os
    mlua_mod_string
    mlua_mod_table
)

# Tool: gen
mlua_add_tool_executable(gen)
target_compile_definitions(gen PRIVATE
    MLUA_MAIN_MODULE=gen
)
target_link_libraries(gen PRIVATE
    gen_main
)
add_executable(mlua_tool_gen ALIAS gen)

# Module: microfs.lfs
mlua_add_c_module(microfs_lfs microfs.lfs.c)
target_include_directories(microfs_lfs_headers INTERFACE
    "${MLUA_LITTLEFS_SOURCE_DIR}"
)
target_compile_definitions(microfs_lfs INTERFACE
    LFS_NO_ASSERT=1
    LFS_NO_DEBUG=1
    LFS_NO_ERROR=1
    LFS_NO_WARN=1
)
target_sources(microfs_lfs INTERFACE
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs.c"
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs_util.c"
)

# Module: microfs
mlua_add_lua_modules(microfs_main microfs.lua)
target_link_libraries(microfs_main INTERFACE
    mlua_mod_io
    mlua_mod_math
    mlua_mod_mlua_block_mem
    mlua_mod_mlua_cli
    mlua_mod_mlua_errors
    mlua_mod_mlua_fs
    mlua_mod_mlua_fs_lfs
    mlua_mod_mlua_io
    mlua_mod_mlua_list
    mlua_mod_mlua_mem
    mlua_mod_mlua_shell
    mlua_mod_mlua_uf2
    mlua_mod_mlua_util
    mlua_mod_os
    mlua_mod_string
    mlua_mod_table
)

# Tool: microfs
mlua_add_tool_executable(microfs)
target_compile_definitions(microfs PRIVATE
    MLUA_MAIN_MODULE=microfs
)
target_link_libraries(microfs PRIVATE
    microfs_main
)
