# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

add_subdirectory(common)
add_subdirectory(pico)

mlua_add_c_module(mlua_mod_mlua_event mlua.event.c)
target_include_directories(mlua_mod_mlua_event_headers INTERFACE
    include_mlua.event)
target_link_libraries(mlua_mod_mlua_event INTERFACE
    hardware_irq
    hardware_sync
    mlua_mod_mlua_int64
    pico_platform
    pico_time
)

mlua_add_c_module(mlua_mod_mlua_fs_loader mlua.fs.loader.c)
target_compile_definitions(mlua_mod_mlua_fs_loader INTERFACE
    LFS_THREADSAFE=1
)
target_link_libraries(mlua_mod_mlua_fs_loader INTERFACE
    hardware_flash
    mlua_mod_mlua_block_flash
    mlua_mod_mlua_errors
    mlua_mod_mlua_fs_headers
    mlua_mod_mlua_fs_lfs
    pico_binary_info
    pico_platform
)

mlua_add_lua_modules(mlua_test_mlua_fs_loader mlua.fs.loader.test.lua)
target_link_libraries(mlua_test_mlua_fs_loader INTERFACE
    mlua_mod_mlua_fs
    mlua_mod_mlua_fs_loader
    mlua_mod_package
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_mlua_stdio mlua.stdio.c)

mlua_add_lua_modules(mlua_test_mlua_stdio mlua.stdio.test.lua)
target_link_libraries(mlua_test_mlua_stdio INTERFACE
    mlua_mod_mlua_io
    mlua_mod_mlua_list
    mlua_mod_mlua_stdio
    mlua_mod_mlua_testing_stdio
    mlua_mod_mlua_util
)

mlua_add_lua_modules(mlua_mod_mlua_testing mlua.testing.lua)
target_link_libraries(mlua_mod_mlua_testing INTERFACE
    mlua_mod_debug
    mlua_mod_math
    mlua_mod_mlua_io
    mlua_mod_mlua_list
    mlua_mod_mlua_oo
    mlua_mod_mlua_util
    mlua_mod_package
    mlua_mod_pico_time
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_testing mlua.testing.test.lua)
target_link_libraries(mlua_test_mlua_testing INTERFACE
    mlua_mod_mlua_list
    mlua_mod_mlua_util
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua_thread mlua.thread.lua)
target_link_libraries(mlua_mod_mlua_thread INTERFACE
    mlua_mod_coroutine
    mlua_mod_mlua_event
    mlua_mod_mlua_oo
    mlua_mod_pico_time
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_test_mlua_thread mlua.thread.test.lua)
target_link_libraries(mlua_test_mlua_thread INTERFACE
    mlua_mod_mlua_thread
    mlua_mod_string
)

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
