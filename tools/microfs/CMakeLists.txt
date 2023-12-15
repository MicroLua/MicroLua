# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

project(microfs C)
set(CMAKE_C_STANDARD 11)

include("${MLUA_PATH}/rules.cmake")

mlua_add_compile_options()
mlua_add_lua_tool(microfs "${CMAKE_CURRENT_LIST_DIR}/microfs.lua")
target_compile_definitions(microfs PRIVATE
    LFS_NO_ASSERT=1
    LFS_NO_DEBUG=1
    LFS_NO_ERROR=1
    LFS_NO_WARN=1
)
target_include_directories(microfs PRIVATE
    "${MLUA_LITTLEFS_SOURCE_DIR}"
)
target_sources(microfs PRIVATE
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs.c"
    "${MLUA_LITTLEFS_SOURCE_DIR}/lfs_util.c"
    lfs.c
)
