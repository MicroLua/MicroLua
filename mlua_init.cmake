# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

if(NOT TARGET mlua_rules_included)
    add_library(mlua_rules_included INTERFACE)
    include("${MLUA_PATH}/rules.cmake")
    mlua_pre_project()
endif()

macro(mlua_init)
    if(NOT CMAKE_PROJECT_NAME)
        message(WARNING "mlua_init() should be called after project()")
    endif()
    mlua_post_project()
    mlua_add_compile_options()
    if(NOT "${CMAKE_CURRENT_LIST_DIR}" STREQUAL "${MLUA_PATH}")
        add_subdirectory("${MLUA_PATH}" MicroLua)
    endif()
endmacro()
