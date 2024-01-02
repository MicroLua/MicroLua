# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

target_include_directories(mlua_platform_luaconf INTERFACE include_luaconf)
target_include_directories(mlua_platform INTERFACE include_platform)
target_sources(mlua_platform INTERFACE platform.c)
target_link_libraries(mlua_platform INTERFACE
    hardware_timer
    mlua_mod_mlua_event_headers
    pico_binary_info
    pico_platform
    pico_time
)
if(NOT "${PICO_BOARD}" STREQUAL "host")
    target_link_libraries(mlua_platform INTERFACE
        hardware_exception
        hardware_flash
    )
endif()

mlua_add_lua_modules(mlua_test_00_setup 00_setup.test.lua)
target_link_libraries(mlua_test_00_setup INTERFACE
    mlua_mod_hardware_uart
    mlua_mod_pico_stdlib
)

mlua_add_c_module(mlua_mod_hardware_adc hardware.adc.c)
target_link_libraries(mlua_mod_hardware_adc INTERFACE
    hardware_adc
    hardware_irq
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware_adc hardware.adc.test.lua)
target_link_libraries(mlua_test_hardware_adc INTERFACE
    mlua_mod_hardware_adc
    mlua_mod_pico_time
)

mlua_add_c_module(mlua_mod_hardware_base hardware.base.c)
target_link_libraries(mlua_mod_hardware_base INTERFACE
    hardware_base
)

mlua_add_lua_modules(mlua_test_hardware_base hardware.base.test.lua)
target_link_libraries(mlua_test_hardware_base INTERFACE
    mlua_mod_hardware_base
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_watchdog
    mlua_mod_mlua_list
)

mlua_add_c_module(mlua_mod_hardware_clocks hardware.clocks.c)
target_link_libraries(mlua_mod_hardware_clocks INTERFACE
    hardware_clocks
)

mlua_add_lua_modules(mlua_test_hardware_clocks hardware.clocks.test.lua)
target_link_libraries(mlua_test_hardware_clocks INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_hardware_regs_clocks
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_hardware_flash hardware.flash.c)
target_link_libraries(mlua_mod_hardware_flash INTERFACE
    hardware_flash
)

mlua_add_lua_modules(mlua_test_hardware_flash hardware.flash.test.lua)
target_link_libraries(mlua_test_hardware_flash INTERFACE
    mlua_mod_hardware_flash
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_sync
    mlua_mod_mlua_mem
    mlua_mod_pico
    mlua_mod_pico_unique_id
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware_gpio hardware.gpio.c)
target_include_directories(mlua_mod_hardware_gpio_headers INTERFACE
    include_hardware.gpio)
target_link_libraries(mlua_mod_hardware_gpio INTERFACE
    hardware_gpio
    hardware_structs
    hardware_sync
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware_gpio hardware.gpio.test.lua)
target_link_libraries(mlua_test_hardware_gpio INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_hardware_irq
    mlua_mod_hardware_regs_io_bank0
    mlua_mod_mlua_list
    mlua_mod_mlua_thread
    mlua_mod_mlua_util
    mlua_mod_pico
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware_i2c hardware.i2c.c)
target_include_directories(mlua_mod_hardware_i2c_headers INTERFACE
    include_hardware.i2c)
target_link_libraries(mlua_mod_hardware_i2c INTERFACE
    hardware_i2c
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    pico_base
    pico_time
)

mlua_add_lua_modules(mlua_test_hardware_i2c hardware.i2c.test.lua)
target_link_libraries(mlua_test_hardware_i2c INTERFACE
    mlua_mod_hardware_base
    mlua_mod_hardware_i2c
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_i2c
    mlua_mod_mlua_testing_i2c
    mlua_mod_mlua_thread
    mlua_mod_pico_multicore
)

mlua_add_c_module(mlua_mod_hardware_irq hardware.irq.c)
target_link_libraries(mlua_mod_hardware_irq INTERFACE
    hardware_irq
    hardware_structs
    hardware_sync
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware_irq hardware.irq.test.lua)
target_link_libraries(mlua_test_hardware_irq INTERFACE
    mlua_mod_hardware_irq
    mlua_mod_mlua_list
    mlua_mod_mlua_thread
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware_pll hardware.pll.c)
target_link_libraries(mlua_mod_hardware_pll INTERFACE
    hardware_pll
)

mlua_add_lua_modules(mlua_test_hardware_pll hardware.pll.test.lua)
target_link_libraries(mlua_test_hardware_pll INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_hardware_regs_clocks
    mlua_mod_hardware_pll
    mlua_mod_mlua_testing_clocks
    mlua_mod_pico_stdlib
)

mlua_add_c_module(mlua_mod_hardware_pwm hardware.pwm.c)
target_link_libraries(mlua_mod_hardware_pwm INTERFACE
    hardware_pwm
    mlua_mod_hardware_gpio_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware_pwm hardware.pwm.test.lua)
target_link_libraries(mlua_test_hardware_pwm INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_hardware_pwm
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_pwm
    mlua_mod_pico
)

mlua_add_lua_modules(mlua_test_hardware_regs hardware.regs.test.lua)

file(GLOB paths LIST_DIRECTORIES false CONFIGURE_DEPENDS
    "${PICO_SDK_PATH}/src/rp2040/hardware_regs/include/hardware/regs/*.h")
foreach(path IN LISTS paths)
    cmake_path(GET path STEM LAST_ONLY name)
    string(TOUPPER "${name}" uname)
    set(target "mlua_mod_hardware_regs_${name}")
    mlua_add_header_module("${target}" "hardware.regs.${name}" "${path}"
        EXCLUDE "_ACCESS:\"...?\"$" "_RESET:\"%-\"$" "^isr_"
        STRIP "^(${uname}_)"
    )
    target_link_libraries("${target}" INTERFACE hardware_regs)
    target_link_libraries(mlua_test_hardware_regs INTERFACE "${target}")
endforeach()

target_link_libraries(mlua_test_hardware_regs INTERFACE
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_intctrl
    mlua_mod_hardware_regs_vreg_and_chip_reset
)

mlua_add_c_module(mlua_mod_hardware_resets hardware.resets.c)
target_link_libraries(mlua_mod_hardware_resets INTERFACE
    hardware_resets
)

mlua_add_lua_modules(mlua_test_hardware_resets hardware.resets.test.lua)
target_link_libraries(mlua_test_hardware_resets INTERFACE
    mlua_mod_hardware_base
    mlua_mod_hardware_regs_adc
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_resets
    mlua_mod_hardware_resets
)

mlua_add_c_module(mlua_mod_hardware_rtc hardware.rtc.c)
target_link_libraries(mlua_mod_hardware_rtc INTERFACE
    hardware_rtc
    mlua_mod_mlua_event_headers
)

mlua_add_lua_modules(mlua_test_hardware_rtc hardware.rtc.test.lua)
target_link_libraries(mlua_test_hardware_rtc INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_hardware_regs_resets
    mlua_mod_hardware_resets
    mlua_mod_hardware_rtc
    mlua_mod_mlua_thread
    mlua_mod_mlua_util
)

mlua_add_c_module(mlua_mod_hardware_spi hardware.spi.c)
target_link_libraries(mlua_mod_hardware_spi INTERFACE
    hardware_base
    hardware_irq
    hardware_regs
    hardware_spi
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware_spi hardware.spi.test.lua)
target_link_libraries(mlua_test_hardware_spi INTERFACE
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_spi
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware_sync hardware.sync.c)
target_link_libraries(mlua_mod_hardware_sync INTERFACE
    hardware_sync
)

mlua_add_lua_modules(mlua_test_hardware_sync hardware.sync.test.lua)
target_link_libraries(mlua_test_hardware_sync INTERFACE
    mlua_mod_hardware_irq
    mlua_mod_hardware_sync
    mlua_mod_mlua_thread
)

mlua_add_c_module(mlua_mod_hardware_timer hardware.timer.c)
target_link_libraries(mlua_mod_hardware_timer INTERFACE
    hardware_timer
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    mlua_mod_mlua_thread
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware_timer hardware.timer.test.lua)
target_link_libraries(mlua_test_hardware_timer INTERFACE
    mlua_mod_hardware_timer
    mlua_mod_mlua_int64
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_hardware_uart hardware.uart.c)
target_include_directories(mlua_mod_hardware_uart_headers INTERFACE
    include_hardware.uart)
target_link_libraries(mlua_mod_hardware_uart INTERFACE
    hardware_base
    hardware_irq
    hardware_regs
    hardware_uart
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    pico_platform
    pico_time
)

mlua_add_lua_modules(mlua_test_hardware_uart hardware.uart.test.lua)
target_link_libraries(mlua_test_hardware_uart INTERFACE
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_uart
    mlua_mod_math
    mlua_mod_mlua_list
    mlua_mod_mlua_testing_uart
    mlua_mod_mlua_thread
    mlua_mod_pico_time
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware_vreg hardware.vreg.c)
target_link_libraries(mlua_mod_hardware_vreg INTERFACE
    hardware_vreg
)

mlua_add_lua_modules(mlua_test_hardware_vreg hardware.vreg.test.lua)
target_link_libraries(mlua_test_hardware_vreg INTERFACE
    mlua_mod_hardware_base
    mlua_mod_hardware_vreg
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_vreg_and_chip_reset
)

mlua_add_c_module(mlua_mod_hardware_watchdog hardware.watchdog.c)
target_link_libraries(mlua_mod_hardware_watchdog INTERFACE
    hardware_watchdog
)

mlua_add_lua_modules(mlua_test_hardware_watchdog hardware.watchdog.test.lua)
target_link_libraries(mlua_test_hardware_watchdog INTERFACE
    mlua_mod_hardware_base
    mlua_mod_hardware_regs_addressmap
    mlua_mod_hardware_regs_psm
    mlua_mod_hardware_regs_watchdog
    mlua_mod_hardware_watchdog
    mlua_mod_pico_time
)

mlua_add_c_module(mlua_mod_hardware_xosc hardware.xosc.c)
target_link_libraries(mlua_mod_hardware_xosc INTERFACE
    hardware_xosc
)

mlua_add_lua_modules(mlua_test_hardware_xosc hardware.xosc.test.lua)
target_link_libraries(mlua_test_hardware_xosc INTERFACE
    mlua_mod_hardware_xosc
)

mlua_add_c_module(mlua_mod_mlua_block_flash mlua.block.flash.c)
target_include_directories(mlua_mod_mlua_block_flash_headers INTERFACE
    include_mlua.block.flash)
target_link_libraries(mlua_mod_mlua_block_flash INTERFACE
    hardware_flash
    hardware_sync
    mlua_mod_mlua_block
    mlua_mod_mlua_errors
)

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

# TODO: Use a platform-specific library
target_link_libraries(mlua_mod_mlua_fs_lfs INTERFACE
    pico_sync
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

mlua_add_lua_modules(mlua_test_mlua_stdio mlua.stdio.test.lua)
target_link_libraries(mlua_test_mlua_stdio INTERFACE
    mlua_mod_mlua_io
    mlua_mod_mlua_list
    mlua_mod_mlua_stdio
    mlua_mod_mlua_testing_stdio
    mlua_mod_mlua_util
)

mlua_add_lua_modules(mlua_mod_mlua_testing_clocks mlua.testing.clocks.lua)
target_link_libraries(mlua_mod_mlua_testing_clocks INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_hardware_uart
    mlua_mod_pico_stdio
    mlua_mod_pico_stdlib
)

mlua_add_lua_modules(mlua_mod_mlua_testing_i2c mlua.testing.i2c.lua)
target_link_libraries(mlua_mod_mlua_testing_i2c INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_hardware_i2c
    mlua_mod_mlua_list
)

mlua_add_lua_modules(mlua_mod_mlua_testing_stdio mlua.testing.stdio.lua)
target_link_libraries(mlua_mod_mlua_testing_stdio INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_hardware_uart
    mlua_mod_mlua_testing_uart
    mlua_mod_pico_stdio
    mlua_mod_pico_stdio_uart
)

mlua_add_lua_modules(mlua_mod_mlua_testing_uart mlua.testing.uart.lua)
target_link_libraries(mlua_mod_mlua_testing_uart INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_hardware_uart
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

mlua_add_c_module(mlua_mod_pico pico.c)
target_link_libraries(mlua_mod_pico INTERFACE
    pico_base
)

mlua_add_lua_modules(mlua_test_pico pico.test.lua)
target_link_libraries(mlua_test_pico INTERFACE
    mlua_mod_pico
)

mlua_add_header_module(mlua_mod_pico_board pico.board
    "${CMAKE_BINARY_DIR}/generated/pico_base/pico/config_autogen.h")
target_link_libraries(mlua_mod_pico INTERFACE
    pico_base
)

mlua_add_lua_modules(mlua_test_pico_board pico.board.test.lua)
target_link_libraries(mlua_test_pico_board INTERFACE
    mlua_mod_pico_board
)

mlua_add_c_module(mlua_mod_pico_i2c_slave pico.i2c_slave.c)
target_link_libraries(mlua_mod_pico_i2c_slave INTERFACE
    hardware_i2c
    mlua_mod_hardware_i2c
    mlua_mod_mlua_event
)

mlua_add_lua_modules(mlua_test_pico_i2c_slave pico.i2c_slave.test.lua)
target_link_libraries(mlua_test_pico_i2c_slave INTERFACE
    mlua_mod_mlua_testing_i2c
    mlua_mod_pico_i2c_slave
)

mlua_add_c_module(mlua_mod_pico_multicore pico.multicore.c)
target_link_libraries(mlua_mod_pico_multicore INTERFACE
    mlua_mod_mlua_event
    pico_multicore
    pico_platform
)

mlua_add_lua_modules(mlua_test_pico_multicore pico.multicore.test.lua)
target_link_libraries(mlua_test_pico_multicore INTERFACE
    mlua_mod_pico_multicore
)

mlua_add_c_module(mlua_mod_pico_multicore_fifo pico.multicore.fifo.c)
target_link_libraries(mlua_mod_pico_multicore_fifo INTERFACE
    hardware_structs
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    pico_multicore
    pico_platform
    pico_time
)

mlua_add_lua_modules(mlua_test_pico_multicore_fifo pico.multicore.fifo.test.lua)
target_link_libraries(mlua_test_pico_multicore_fifo INTERFACE
    mlua_mod_mlua_thread
    mlua_mod_pico_multicore
    mlua_mod_pico_multicore_fifo
)

mlua_add_c_module(mlua_mod_pico_platform pico.platform.c)
target_link_libraries(mlua_mod_pico_platform INTERFACE
    pico_platform
)

mlua_add_lua_modules(mlua_test_pico_platform pico.platform.test.lua)
target_link_libraries(mlua_test_pico_platform INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_math
    mlua_mod_pico_platform
    mlua_mod_pico_time
)

mlua_add_c_module(mlua_mod_pico_stdio pico.stdio.c)
target_include_directories(mlua_mod_pico_stdio_headers INTERFACE
    include_pico.stdio)
target_link_libraries(mlua_mod_pico_stdio INTERFACE
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    pico_stdio
    pico_time
)

mlua_add_lua_modules(mlua_test_pico_stdio pico.stdio.test.lua)
target_link_libraries(mlua_test_pico_stdio INTERFACE
    mlua_mod_mlua_list
    mlua_mod_mlua_testing_stdio
    mlua_mod_mlua_thread
    mlua_mod_mlua_util
    mlua_mod_pico_stdio
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_pico_stdio_semihosting pico.stdio.semihosting.c)
target_link_libraries(mlua_mod_pico_stdio_semihosting INTERFACE
    hardware_exception
    pico_stdio_semihosting
)

mlua_add_lua_modules(mlua_test_pico_stdio_semihosting
    pico.stdio.semihosting.test.lua)
target_link_libraries(mlua_test_pico_stdio_semihosting INTERFACE
    mlua_mod_pico_stdio
    mlua_mod_pico_stdio_semihosting
)

mlua_add_c_module(mlua_mod_pico_stdio_uart pico.stdio.uart.c)
target_link_libraries(mlua_mod_pico_stdio_uart INTERFACE
    mlua_mod_hardware_uart_headers
    pico_stdio_uart
)

mlua_add_lua_modules(mlua_test_pico_stdio_uart pico.stdio.uart.test.lua)
target_link_libraries(mlua_test_pico_stdio_uart INTERFACE
    mlua_mod_hardware_gpio
    mlua_mod_hardware_uart
    mlua_mod_mlua_list
    mlua_mod_mlua_testing_uart
    mlua_mod_pico
    mlua_mod_pico_stdio_uart
)

mlua_add_c_module(mlua_mod_pico_stdio_usb pico.stdio.usb.c)
target_link_libraries(mlua_mod_pico_stdio_usb INTERFACE
    pico_stdio_usb
)

mlua_add_c_module(mlua_mod_pico_stdlib pico.stdlib.c)
target_link_libraries(mlua_mod_pico_stdlib INTERFACE
    pico_stdlib
)

mlua_add_lua_modules(mlua_test_pico_stdlib pico.stdlib.test.lua)
target_link_libraries(mlua_test_pico_stdlib INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_mlua_testing_clocks
    mlua_mod_pico_stdlib
)

mlua_add_c_module(mlua_mod_pico_time pico.time.c)
target_link_libraries(mlua_mod_pico_time INTERFACE
    hardware_timer
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    pico_time
)

mlua_add_lua_modules(mlua_test_pico_time pico.time.test.lua)
target_link_libraries(mlua_test_pico_time INTERFACE
    mlua_mod_mlua_int64
    mlua_mod_mlua_list
    mlua_mod_mlua_thread
    mlua_mod_pico_time
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_pico_unique_id pico.unique_id.c)
target_link_libraries(mlua_mod_pico_unique_id INTERFACE
    pico_unique_id
)

mlua_add_lua_modules(mlua_test_pico_unique_id pico.unique_id.test.lua)
target_link_libraries(mlua_test_pico_unique_id INTERFACE
    mlua_mod_pico_unique_id
    mlua_mod_string
)
