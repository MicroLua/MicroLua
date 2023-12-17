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

include(ExternalProject)

ExternalProject_Add(mlua_tools
    SOURCE_DIR "${MLUA_PATH}/tools"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tools"
    CMAKE_ARGS
        "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
        "-DMLUA_PATH:PATH=${MLUA_PATH}"
        "-DMLUA_INT:STRING=${MLUA_INT}"
        "-DMLUA_FLOAT:STRING=${MLUA_FLOAT}"
        "-DPICO_PLATFORM:STRING=host"
    BUILD_ALWAYS 1  # Force dependency checking
    INSTALL_COMMAND ""
)

mlua_add_tool(mlua_tool_gen "${CMAKE_CURRENT_BINARY_DIR}/tools/gen")
