# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

project(tools C)
set(CMAKE_C_STANDARD 11)

include("${MLUA_PATH}/rules.cmake")

mlua_add_compile_options()

mlua_add_lua_tool(gen "${CMAKE_CURRENT_LIST_DIR}/gen.lua")
add_executable(mlua_tool_gen ALIAS gen)

add_subdirectory("${MLUA_PATH}/core" core)

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

mlua_add_lua_modules(microfs_main microfs.lua)
target_link_libraries(microfs_main INTERFACE
    microfs_lfs
    mlua_mod_io
    mlua_mod_string
)

mlua_add_tool_executable(microfs)
target_compile_definitions(microfs PRIVATE
    MLUA_MAIN_MODULE=microfs
)
target_link_libraries(microfs PRIVATE
    microfs_main
)
