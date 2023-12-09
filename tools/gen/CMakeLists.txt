# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

project(mlua C)
set(CMAKE_C_STANDARD 11)

include("${MLUA_PATH}/rules.cmake")

mlua_add_compile_options()
mlua_add_lua_tool(gen "${CMAKE_CURRENT_LIST_DIR}/gen.lua")
