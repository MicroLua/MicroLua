# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

macro(mlua_pre_project)
    if(NOT DEFINED ENV{PICO_SDK_PATH} AND NOT PICO_SDK_PATH)
        set(PICO_SDK_PATH "${MLUA_PATH}/ext/pico-sdk" CACHE PATH "")
    endif()
    set(PICO_LWIP_PATH "${MLUA_LWIP_SOURCE_DIR}")
    include("${MLUA_PATH}/lib/pico/pico_sdk_import.cmake")
endmacro()

macro(mlua_post_project)
    if(PICO_SDK_VERSION_STRING VERSION_LESS "2.1.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.1.0 (or later) required, found ${PICO_SDK_VERSION_STRING}")
    endif()
    pico_sdk_init()
    add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)
endmacro()

function(mlua_add_executable_platform TARGET)
    pico_add_extra_outputs("${TARGET}")
endfunction()
