# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.24)

if(NOT MLUA_PATH)
    set(MLUA_PATH "${CMAKE_CURRENT_LIST_DIR}")
endif()
include(mlua_import.cmake)

project(microlua C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

mlua_init()
add_subdirectory(core)
add_subdirectory(lib)
add_subdirectory(tools)
if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(bin)
endif()
