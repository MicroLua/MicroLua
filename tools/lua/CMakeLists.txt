# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

project(lua C)
set(CMAKE_C_STANDARD 11)

include("${MLUA_PATH}/rules.cmake")

mlua_core_filenames(srcs *.[ch])
list(FILTER srcs EXCLUDE REGEX ^luaconf\\.h$)
mlua_core_copy("${srcs}" src)
mlua_core_luaconf(src)

add_executable(lua "${CMAKE_CURRENT_BINARY_DIR}/src/onelua.c")
target_compile_definitions(lua PRIVATE
    MAKE_LUA
    MLUA_MAXSTACK=0
    MLUA_BUFFERSIZE=0
)
target_link_libraries(lua PRIVATE m)
if(CMAKE_HOST_WIN32)
    target_compile_definitions(lua PRIVATE LUA_USE_WINDOWS)
else()
    target_compile_definitions(lua PRIVATE LUA_USE_LINUX)
    target_link_libraries(lua PRIVATE dl)
endif()
