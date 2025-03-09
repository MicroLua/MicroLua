# Copyright 2025 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.24)

include("${MLUA_PATH}/mlua_import.cmake")

project(gen C)
set(CMAKE_C_STANDARD 11)

set(MLUA_BUILDING_TOOL gen)
mlua_init()

# Module: gen
mlua_add_lua_modules(gen_main NOCOMPILE gen.lua)
target_link_libraries(gen_main INTERFACE
    mlua_mod_io
    mlua_mod_os
    mlua_mod_string
    mlua_mod_table
)

# Executable: gen
mlua_add_executable_no_config(gen)
add_executable(mlua_tool_gen ALIAS gen)
target_compile_definitions(gen PRIVATE
    MLUA_MAIN_MODULE=gen
)
target_link_libraries(gen PRIVATE
    gen_main
)
