# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

add_subdirectory(common)
add_subdirectory(pico)

# Executable: standalone binary
add_executable(mlua_standalone)
mlua_add_config_module(mlua_standalone)
target_compile_definitions(mlua_standalone PRIVATE
    MLUA_FS_LOADER_FLAT=1
    MLUA_FS_LOADER_SIZE=1<<20
    MLUA_STDIO_INIT_UART_IN=1
    MLUA_STDIO_INIT_UART_OUT=1
    MLUA_STDIO_INIT_USB=1
    MLUA_SYMBOL_HASH_DEBUG=0
    PICO_ENTER_USB_BOOT_ON_EXIT=1
    PICO_MAX_SHARED_IRQ_HANDLERS=16
    PICO_PROGRAM_NAME="MicroLua"
    PICO_PROGRAM_URL="https://github.com/MicroLua/MicroLua"
    PICO_PROGRAM_VERSION_STRING="0.0"
)
mlua_target_config(mlua_standalone
)
target_link_libraries(mlua_standalone PRIVATE
    mlua_mod_mlua_stdio
    mlua_mod_mlua_thread
    pico_stdlib
)
mlua_list_targets(modules
    DIRS "${MLUA_PATH}/core" "${MLUA_PATH}/lib"
    INCLUDE "^mlua_mod_.*$"
    EXCLUDE "^mlua_mod_.*_headers$"
            "^mlua_mod_mlua_testing_.*$"
)
list(SORT modules COMPARE NATURAL)
target_link_libraries(mlua_standalone PRIVATE ${modules})
if(COMMAND pico_add_extra_outputs)
    pico_add_extra_outputs(mlua_standalone)
endif()

# Executable: unit tests
add_executable(mlua_tests)
mlua_add_config_module(mlua_tests)
target_compile_definitions(mlua_tests PRIVATE
    MLUA_FS_LOADER_FLAT=1
    MLUA_FS_LOADER_SIZE=100<<10
    MLUA_MAIN_MODULE=mlua.testing
    MLUA_STDIO_INIT_UART_IN=1
    MLUA_STDIO_INIT_UART_OUT=1
    MLUA_SYMBOL_HASH_DEBUG=0
    PICO_ENTER_USB_BOOT_ON_EXIT=1
    PICO_MAX_SHARED_IRQ_HANDLERS=16
    PICO_PROGRAM_NAME="MicroLua-tests"
)
mlua_target_config(mlua_tests
    HASH_SYMBOL_TABLES:integer=MLUA_HASH_SYMBOL_TABLES
    I2C_MASTER_SCL:integer=5
    I2C_MASTER_SDA:integer=4
    I2C_SLAVE_SCL:integer=7
    I2C_SLAVE_SDA:integer=6
    a:integer=1
    b:string="test"
)
target_link_libraries(mlua_tests PRIVATE
    mlua_mod_mlua_stdio
    mlua_mod_mlua_testing
    mlua_mod_mlua_thread
    pico_stdlib
)
mlua_list_targets(tests
    DIRS "${MLUA_PATH}/core" "${MLUA_PATH}/lib"
    INCLUDE "^mlua_test_.*$"
)
list(SORT tests COMPARE NATURAL)
target_link_libraries(mlua_tests PRIVATE ${tests})
if(COMMAND pico_add_extra_outputs)
    pico_add_extra_outputs(mlua_tests)
endif()
