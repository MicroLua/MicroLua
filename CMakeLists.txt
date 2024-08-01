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
if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(bin)
endif()

if(NOT MLUA_HAS_GEN)
    include(ExternalProject)
    ExternalProject_Add(mlua_tools
        SOURCE_DIR "${MLUA_PATH}/tools"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/tools"
        CMAKE_ARGS
            "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
            "-DMLUA_PATH:PATH=${MLUA_PATH}"
            "-DMLUA_PLATFORM:STRING=host"
            "-DMLUA_INT:STRING=${MLUA_INT}"
            "-DMLUA_FLOAT:STRING=${MLUA_FLOAT}"
            "-DMLUA_FENNEL:STRING=${MLUA_FENNEL}"
        BUILD_ALWAYS 1  # Force dependency checking
        INSTALL_COMMAND ""
    )
    mlua_add_tool(mlua_tool_gen "${CMAKE_CURRENT_BINARY_DIR}/tools/gen")
endif()
