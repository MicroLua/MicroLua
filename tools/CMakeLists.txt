# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

include("${MLUA_PATH}/mlua_import.cmake")

project(tools C)
set(CMAKE_C_STANDARD 11)

set(MLUA_HAS_GEN true)
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

# Module: mlua_cli
mlua_add_lua_modules(mlua_cli mlua_cli.lua)
target_link_libraries(mlua_cli INTERFACE
    mlua_mod_io
    mlua_mod_math
    mlua_mod_mlua.block.mem
    mlua_mod_mlua.cli
    mlua_mod_mlua.fs
    mlua_mod_mlua.fs.lfs
    mlua_mod_mlua.io
    mlua_mod_mlua.list
    mlua_mod_mlua.mem
    mlua_mod_mlua.shell
    mlua_mod_mlua.uf2
    mlua_mod_mlua.util
    mlua_mod_os
    mlua_mod_string
    mlua_mod_table
)

# Executable: mlua
mlua_add_executable(mlua)
target_compile_definitions(mlua PRIVATE
    MLUA_MAIN_MODULE=mlua_cli
)
target_link_libraries(mlua PRIVATE
    mlua_cli
)
