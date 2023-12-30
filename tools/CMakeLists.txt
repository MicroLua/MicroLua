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
add_executable(mlua_tool_gen ALIAS gen)
target_compile_definitions(gen PRIVATE
    MLUA_MAIN_MODULE=gen
)
target_link_libraries(gen PRIVATE
    gen_main
)

# Module: microlua
mlua_add_lua_modules(microlua_main microlua.lua)
target_link_libraries(microlua_main INTERFACE
    mlua_mod_io
    mlua_mod_math
    mlua_mod_mlua_block_mem
    mlua_mod_mlua_cli
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

# Tool: microlua
mlua_add_tool_executable(microlua)
target_compile_definitions(microlua PRIVATE
    MLUA_MAIN_MODULE=microlua
)
target_link_libraries(microlua PRIVATE
    microlua_main
)
