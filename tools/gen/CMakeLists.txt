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

# Embed the script.
set(SCRIPT "${CMAKE_CURRENT_LIST_DIR}/gen.lua")
cmake_path(GET SCRIPT FILENAME FILE)
configure_file(tool_main.in.c "${CMAKE_CURRENT_BINARY_DIR}/main.c")
set_source_files_properties(
    "${CMAKE_CURRENT_BINARY_DIR}/main.c"
    OBJECT_DEPENDS "${SCRIPT}"
)

add_executable(gen
    "${CMAKE_BINARY_DIR}/ext/lua/onelua.c"
    "${CMAKE_CURRENT_BINARY_DIR}/main.c"
)
target_compile_definitions(gen PRIVATE
    MAKE_LIB
)
target_include_directories(gen PRIVATE
    "${CMAKE_BINARY_DIR}/ext/lua"
)
target_link_libraries(gen PRIVATE m)
if(CMAKE_HOST_WIN32)
    target_compile_definitions(gen PRIVATE LUA_USE_WINDOWS)
else()
    target_compile_definitions(gen PRIVATE LUA_USE_LINUX)
endif()
