# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

macro(mlua_pre_project)
    include("${MLUA_PATH}/lib/pico/pico_sdk_import.cmake")
endmacro()

macro(mlua_post_project)
    if(PICO_SDK_VERSION_STRING VERSION_LESS "1.5.1")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.1 (or later) required, found ${PICO_SDK_VERSION_STRING}")
    endif()
    pico_sdk_init()
    add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)
endmacro()

function(mlua_add_executable_platform TARGET)
    pico_add_extra_outputs("${TARGET}")
endfunction()
