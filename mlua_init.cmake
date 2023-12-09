# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

include("${MLUA_PATH}/pico_sdk_import.cmake")

macro(mlua_init)
    if(NOT CMAKE_PROJECT_NAME)
        message(WARNING
            "mlua_init() should be called after the project is created")
    endif()
    pico_sdk_init()
    add_subdirectory("${MLUA_PATH}" MicroLua)
    mlua_add_compile_options()
endmacro()
