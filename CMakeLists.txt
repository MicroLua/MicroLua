cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(microlua C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(PICO_SDK_VERSION_STRING VERSION_LESS "1.5.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.5.0 (or later) required, found ${PICO_SDK_VERSION_STRING}.")
endif()
pico_sdk_init()

add_subdirectory(core)
add_subdirectory(examples)

add_compile_options(
    -Wall
    -Werror
    -Wextra
    -Wsign-compare
)
