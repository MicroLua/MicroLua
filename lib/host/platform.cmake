# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

macro(mlua_pre_project)
endmacro()

macro(mlua_post_project)
    foreach(LANG IN ITEMS C CXX ASM)
        set("CMAKE_${LANG}_FLAGS" "${CMAKE_${LANG}_FLAGS} -march=native")
        set("CMAKE_${LANG}_FLAGS_RELEASE" "${CMAKE_${LANG}_FLAGS_RELEASE} -O2")
    endforeach()
endmacro()

function(mlua_add_executable_platform TARGET)
    target_compile_definitions("${TARGET}" PRIVATE LUA_USE_POSIX)
    target_link_libraries("${TARGET}" PRIVATE m)
endfunction()
