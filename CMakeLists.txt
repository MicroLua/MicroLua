# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

include(pico_sdk_import.cmake)

project(microlua C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required, found ${PICO_SDK_VERSION_STRING}.")
endif()
pico_sdk_init()

if(NOT MLUA_PATH)
    set(MLUA_PATH "${CMAKE_CURRENT_LIST_DIR}")
endif()

include(rules.cmake)

mlua_add_compile_options()
add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)

add_subdirectory(core)
add_subdirectory(lib)
add_subdirectory(tools)
