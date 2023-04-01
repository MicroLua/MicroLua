cmake_minimum_required(VERSION 3.13)

project(lua C)
set(CMAKE_C_STANDARD 11)

add_executable(lua ${CMAKE_SOURCE_DIR}/../../ext/lua/onelua.c)
target_compile_definitions(lua PRIVATE LUA_USE_LINUX MAKE_LUA)
target_link_options(lua PRIVATE -ldl -lm)
