# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.20)

project(mlua C)
set(CMAKE_C_STANDARD 11)

include("${MLUA_PATH}/rules.cmake")

add_compile_options(
    -Wall -Werror -Wextra -Wsign-compare -Wdouble-promotion
    -Wno-unused-function -Wno-unused-parameter
)

# Copy Lua headers and generate luaconf.h.
mlua_core_filenames(lua_h "*.h")
list(FILTER lua_h EXCLUDE REGEX "^luaconf\\.h$")
mlua_core_copy("${lua_h}" include)
mlua_core_luaconf(include)

# Copy Lua sources.
mlua_core_filenames(lua_c "*.c")
mlua_core_copy("${lua_c}" src)

# Embed the script.
set(SCRIPT "${CMAKE_CURRENT_LIST_DIR}/gen.lua")
cmake_path(GET SCRIPT FILENAME FILE)
configure_file(tool_main.in.c "${CMAKE_CURRENT_BINARY_DIR}/main.c")
set_source_files_properties(
    "${CMAKE_CURRENT_BINARY_DIR}/main.c"
    OBJECT_DEPENDS "${SCRIPT}"
)

add_executable(gen
    "${CMAKE_CURRENT_BINARY_DIR}/src/onelua.c"
    "${CMAKE_CURRENT_BINARY_DIR}/main.c"
)
target_compile_definitions(gen PRIVATE
    MAKE_LIB
    MLUA_MAXSTACK=0
    MLUA_BUFFERSIZE=0
)
target_include_directories(gen PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}/include"
)
target_link_libraries(gen PRIVATE m)
if(CMAKE_HOST_WIN32)
    target_compile_definitions(gen PRIVATE LUA_USE_WINDOWS)
else()
    target_compile_definitions(gen PRIVATE LUA_USE_LINUX)
endif()
