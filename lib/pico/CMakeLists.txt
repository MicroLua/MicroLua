# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

mlua_set(MLUA_TEST_WIFI_SSID "" CACHE STRING "Wi-Fi SSID for unit tests")
mlua_set(MLUA_TEST_WIFI_PASSWORD "" CACHE STRING
         "Wi-Fi password for unit tests")
mlua_set(MLUA_TEST_SERVER_ADDR "" CACHE STRING "Server address for unit tests")
mlua_set(MLUA_TEST_SERVER_PORT "8000" CACHE STRING "Server port for unit tests")

# Library: platform abstraction layer
target_include_directories(mlua_platform_luaconf INTERFACE include_luaconf)
target_include_directories(mlua_platform_headers INTERFACE include_platform)
target_sources(mlua_platform INTERFACE platform.c)
mlua_mirrored_target_link_libraries(mlua_platform INTERFACE
    hardware_sync
    hardware_timer
    pico_async_context_threadsafe_background
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

# Executable: standalone binary
function(mlua_platform_bin_standalone TARGET)
    target_compile_definitions("${TARGET}" PRIVATE
        MLUA_FS_LOADER_FLAT=1
        MLUA_FS_LOADER_SIZE=1<<20
        MLUA_STDIO_INIT_USB=1
        PICO_ENTER_USB_BOOT_ON_EXIT=1
        PICO_MAX_SHARED_IRQ_HANDLERS=16
        PICO_PROGRAM_NAME="MicroLua"
        PICO_PROGRAM_URL="https://github.com/MicroLua/MicroLua"
        PICO_PROGRAM_VERSION_STRING="0.0"
        PICO_STACK_SIZE=0x1000
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
        PICO_USE_STACK_GUARDS=1
    )
    target_link_libraries("${TARGET}" PRIVATE
        pico_stdlib
    )
endfunction()

# Executable: unit tests
function(mlua_platform_bin_tests TARGET SUFFIX)
    target_compile_definitions("${TARGET}" PRIVATE
        LWIP_DHCP=1
        LWIP_ETHARP_STATS=0
        LWIP_HAVE_LOOPIF=1
        LWIP_ICMP_STATS=0
        LWIP_ICMP6_STATS=0
        LWIP_IGMP_STATS=1
        LWIP_IP_STATS=0
        LWIP_IP6_FRAG_STATS=0
        LWIP_IP6_STATS=0
        LWIP_IPFRAG_STATS=0
        LWIP_IPV4=1
        LWIP_IPV6=1
        LWIP_LINK_STATS=1
        LWIP_LOOPIF_MULTICAST=1
        LWIP_MEM_SIZE=10000
        LWIP_MEM_STATS=1
        LWIP_MEMP_NUM_TCP_PCB=10
        LWIP_MEMP_STATS=1
        LWIP_MIB2_STATS=1
        LWIP_MLD6_STATS=0
        LWIP_ND6_STATS=0
        LWIP_NETIF_LOOPBACK=1
        LWIP_PBUF_POOL_SIZE=24
        LWIP_STATS_DISPLAY=1
        LWIP_TCP_STATS=0
        LWIP_UDP_STATS=0
        MLUA_FS_LOADER_FLAT=1
        MLUA_FS_LOADER_SIZE=100<<10
        MLUA_STDIO_INIT_UART_IN=1
        MLUA_STDIO_INIT_UART_OUT=1
        MLUA_STDIO_INIT_USB=1
        PICO_ENTER_USB_BOOT_ON_EXIT=1
        PICO_HEAP_SIZE=0x20000
        PICO_MAX_SHARED_IRQ_HANDLERS=16
        PICO_PROGRAM_NAME="MicroLua-tests${SUFFIX}"
        PICO_STACK_SIZE=0x1000
        PICO_STDIO_USB_CONNECT_WAIT_TIMEOUT_MS=1000
        PICO_USE_STACK_GUARDS=1
    )
    mlua_target_config("${TARGET}"
        GPIO_PIN1:integer=2
        GPIO_PIN2:integer=3
        I2C_MASTER_SCL:integer=5
        I2C_MASTER_SDA:integer=4
        I2C_SLAVE_SCL:integer=7
        I2C_SLAVE_SDA:integer=6
        SERVER_ADDR:string="${MLUA_TEST_SERVER_ADDR}"
        SERVER_PORT:integer=${MLUA_TEST_SERVER_PORT}
        WIFI_PASSWORD:string="${MLUA_TEST_WIFI_PASSWORD}"
        WIFI_SSID:string="${MLUA_TEST_WIFI_SSID}"
    )
    target_link_libraries("${TARGET}" PRIVATE
        mlua_mod_pico.stdio
        mlua_mod_pico.stdio.uart
        mlua_mod_pico.stdio.usb
        pico_stdlib
    )
endfunction()

mlua_add_lua_modules(mlua_test-ALL_00_setup 00_setup.test.lua)
target_link_libraries(mlua_test-ALL_00_setup INTERFACE
    mlua_mod_hardware.clocks
    mlua_mod_hardware.gpio
    mlua_mod_hardware.uart
    mlua_mod_mlua.testing.uart
    mlua_mod_pico.stdio
)

mlua_add_lua_modules(mlua_test-net_10_setup 10_setup.test.lua)
target_link_libraries(mlua_test-net_10_setup INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.ip6
    mlua_mod_lwip.netif
    mlua_mod_mlua.time
    mlua_mod_pico.cyw43
    mlua_mod_pico.cyw43.util
    mlua_mod_pico.cyw43.wifi
)

mlua_add_c_module(mlua_mod_hardware.adc hardware.adc.c)
target_link_libraries(mlua_mod_hardware.adc INTERFACE
    hardware_adc
    hardware_irq
    mlua_mod_mlua.thread_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.adc hardware.adc.test.lua)
target_link_libraries(mlua_test_hardware.adc INTERFACE
    mlua_mod_hardware.adc
    mlua_mod_mlua.thread
    mlua_mod_mlua.time
)

mlua_add_c_module(mlua_mod_hardware.base hardware.base.c)
target_link_libraries(mlua_mod_hardware.base INTERFACE
    hardware_base
)

mlua_add_lua_modules(mlua_test_hardware.base hardware.base.test.lua)
target_link_libraries(mlua_test_hardware.base INTERFACE
    mlua_mod_hardware.base
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.watchdog
    mlua_mod_mlua.list
)

mlua_add_c_module(mlua_mod_hardware.clocks hardware.clocks.c)
target_link_libraries(mlua_mod_hardware.clocks INTERFACE
    hardware_clocks
)

mlua_add_lua_modules(mlua_test_hardware.clocks hardware.clocks.test.lua)
target_link_libraries(mlua_test_hardware.clocks INTERFACE
    mlua_mod_hardware.clocks
    mlua_mod_hardware.regs.clocks
    mlua_mod_mlua.testing.clocks
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_hardware.dma hardware.dma.c)
target_link_libraries(mlua_mod_hardware.dma INTERFACE
    hardware_base
    hardware_dma
    mlua_mod_mlua.thread_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.dma hardware.dma.test.lua)
target_link_libraries(mlua_test_hardware.dma INTERFACE
    mlua_mod_hardware.dma
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.dma
    mlua_mod_hardware.regs.dreq
    mlua_mod_mlua.mem
    mlua_mod_mlua.thread
    mlua_mod_mlua.time
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.flash hardware.flash.c)
target_link_libraries(mlua_mod_hardware.flash INTERFACE
    hardware_flash
)

mlua_add_lua_modules(mlua_test_hardware.flash hardware.flash.test.lua)
target_link_libraries(mlua_test_hardware.flash INTERFACE
    mlua_mod_hardware.flash
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.sync
    mlua_mod_mlua.mem
    mlua_mod_pico
    mlua_mod_pico.unique_id
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.gpio hardware.gpio.c)
target_include_directories(mlua_mod_hardware.gpio_headers INTERFACE
    include_hardware.gpio)
mlua_mirrored_target_link_libraries(mlua_mod_hardware.gpio INTERFACE
    pico_platform
)
target_link_libraries(mlua_mod_hardware.gpio INTERFACE
    hardware_gpio
    hardware_structs
    hardware_sync
    mlua_mod_mlua.thread_headers
)

mlua_add_lua_modules(mlua_test_hardware.gpio hardware.gpio.test.lua)
target_link_libraries(mlua_test_hardware.gpio INTERFACE
    mlua_mod_hardware.gpio
    mlua_mod_hardware.irq
    mlua_mod_hardware.regs.io_bank0
    mlua_mod_mlua.list
    mlua_mod_mlua.thread
    mlua_mod_mlua.util
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.i2c hardware.i2c.c)
target_include_directories(mlua_mod_hardware.i2c_headers INTERFACE
    include_hardware.i2c)
target_link_libraries(mlua_mod_hardware.i2c_headers INTERFACE
    mlua_mod_mlua.thread_headers
)
mlua_mirrored_target_link_libraries(mlua_mod_hardware.i2c INTERFACE
    hardware_i2c
)
target_link_libraries(mlua_mod_hardware.i2c INTERFACE
    mlua_mod_mlua.int64
    pico_base
    pico_time
)

mlua_add_lua_modules(mlua_test_hardware.i2c hardware.i2c.test.lua)
target_link_libraries(mlua_test_hardware.i2c INTERFACE
    mlua_mod_hardware.base
    mlua_mod_hardware.i2c
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.i2c
    mlua_mod_mlua.testing.i2c
    mlua_mod_mlua.thread
    mlua_mod_pico.multicore
)

mlua_add_c_module(mlua_mod_hardware.irq hardware.irq.c)
target_link_libraries(mlua_mod_hardware.irq INTERFACE
    hardware_irq
    hardware_structs
    hardware_sync
    mlua_mod_mlua.thread_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.irq hardware.irq.test.lua)
target_link_libraries(mlua_test_hardware.irq INTERFACE
    mlua_mod_hardware.irq
    mlua_mod_mlua.list
    mlua_mod_mlua.thread
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.pio hardware.pio.c)
target_link_libraries(mlua_mod_hardware.pio INTERFACE
    hardware_base
    hardware_irq
    hardware_pio
    hardware_sync
    mlua_mod_mlua.thread_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.pio hardware.pio.test.lua)
target_link_libraries(mlua_test_hardware.pio INTERFACE
    mlua_mod_hardware.pio
    mlua_mod_hardware.regs.addressmap
    mlua_mod_mlua.list
    mlua_mod_mlua.thread
    mlua_mod_mlua.time
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_hardware.pio.asm hardware.pio.asm.lua)
target_link_libraries(mlua_mod_hardware.pio.asm INTERFACE
    mlua_mod_hardware.pio
    mlua_mod_mlua.oo
    mlua_mod_string
)

mlua_add_lua_modules(mlua_test_hardware.pio.asm hardware.pio.asm.test.lua)
target_link_libraries(mlua_test_hardware.pio.asm INTERFACE
    mlua_mod_hardware.pio
    mlua_mod_hardware.pio.asm
    mlua_mod_mlua.list
    mlua_mod_mlua.util
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.pll hardware.pll.c)
target_link_libraries(mlua_mod_hardware.pll INTERFACE
    hardware_pll
)

mlua_add_lua_modules(mlua_test_hardware.pll hardware.pll.test.lua)
target_link_libraries(mlua_test_hardware.pll INTERFACE
    mlua_mod_hardware.clocks
    mlua_mod_hardware.regs.clocks
    mlua_mod_hardware.pll
    mlua_mod_mlua.testing.clocks
)

mlua_add_c_module(mlua_mod_hardware.pwm hardware.pwm.c)
target_link_libraries(mlua_mod_hardware.pwm INTERFACE
    hardware_irq
    hardware_pwm
    mlua_mod_hardware.gpio_headers
    mlua_mod_mlua.thread_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.pwm hardware.pwm.test.lua)
target_link_libraries(mlua_test_hardware.pwm INTERFACE
    mlua_mod_hardware.gpio
    mlua_mod_hardware.pwm
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.pwm
    mlua_mod_mlua.thread
    mlua_mod_mlua.util
    mlua_mod_pico
)

mlua_add_lua_modules(mlua_test_hardware.regs hardware.regs.test.lua)

file(GLOB paths LIST_DIRECTORIES false CONFIGURE_DEPENDS
    "${PICO_SDK_PATH}/src/rp2040/hardware_regs/include/hardware/regs/*.h")
foreach(path IN LISTS paths)
    cmake_path(GET path STEM LAST_ONLY name)
    string(TOUPPER "${name}" uname)
    set(target "mlua_mod_hardware.regs.${name}")
    if("${name}" STREQUAL addressmap)
        mlua_add_header_module("${target}" "hardware.regs.${name}" "${path}"
            EXCLUDE "^NUM_" "^PIO_INSTRUCTION_" "^USBCTRL_" "^HAS_"
                    "_HZ:" "_[KM]HZ:" "_IRQ:"
            TYPES "_BITS:=integer" "=lightuserdata:(void*)(uintptr_t)"
        )
    else()
        mlua_add_header_module("${target}" "hardware.regs.${name}" "${path}"
            EXCLUDE "_ACCESS:\"...?\"$" "_RESET:\"%-\"$" "^isr_"
            STRIP "^(${uname}_)"
        )
    endif()
    target_link_libraries("${target}" INTERFACE hardware_regs)
    target_link_libraries(mlua_test_hardware.regs INTERFACE "${target}")
endforeach()

target_link_libraries(mlua_test_hardware.regs INTERFACE
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.intctrl
    mlua_mod_hardware.regs.vreg_and_chip_reset
)

mlua_add_c_module(mlua_mod_hardware.resets hardware.resets.c)
target_link_libraries(mlua_mod_hardware.resets INTERFACE
    hardware_resets
)

mlua_add_lua_modules(mlua_test_hardware.resets hardware.resets.test.lua)
target_link_libraries(mlua_test_hardware.resets INTERFACE
    mlua_mod_hardware.base
    mlua_mod_hardware.regs.adc
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.resets
    mlua_mod_hardware.resets
)

mlua_add_c_module(mlua_mod_hardware.rtc hardware.rtc.c)
target_link_libraries(mlua_mod_hardware.rtc INTERFACE
    hardware_rtc
    mlua_mod_mlua.thread_headers
)

mlua_add_lua_modules(mlua_test_hardware.rtc hardware.rtc.test.lua)
target_link_libraries(mlua_test_hardware.rtc INTERFACE
    mlua_mod_hardware.clocks
    mlua_mod_hardware.regs.resets
    mlua_mod_hardware.resets
    mlua_mod_hardware.rtc
    mlua_mod_mlua.thread
    mlua_mod_mlua.util
)

mlua_add_c_module(mlua_mod_hardware.spi hardware.spi.c)
target_link_libraries(mlua_mod_hardware.spi INTERFACE
    hardware_base
    hardware_irq
    hardware_regs
    hardware_spi
    mlua_mod_mlua.thread_headers
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.spi hardware.spi.test.lua)
target_link_libraries(mlua_test_hardware.spi INTERFACE
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.spi
    mlua_mod_mlua.thread
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.sync hardware.sync.c)
target_link_libraries(mlua_mod_hardware.sync INTERFACE
    hardware_sync
)

mlua_add_lua_modules(mlua_test_hardware.sync hardware.sync.test.lua)
target_link_libraries(mlua_test_hardware.sync INTERFACE
    mlua_mod_hardware.irq
    mlua_mod_hardware.sync
    mlua_mod_mlua.thread
)

mlua_add_c_module(mlua_mod_hardware.timer hardware.timer.c)
target_link_libraries(mlua_mod_hardware.timer INTERFACE
    hardware_timer
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread
    pico_platform
)

mlua_add_lua_modules(mlua_test_hardware.timer hardware.timer.test.lua)
target_link_libraries(mlua_test_hardware.timer INTERFACE
    mlua_mod_hardware.timer
    mlua_mod_math
    mlua_mod_mlua.int64
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_hardware.uart hardware.uart.c)
target_include_directories(mlua_mod_hardware.uart_headers INTERFACE
    include_hardware.uart)
mlua_mirrored_target_link_libraries(mlua_mod_hardware.uart INTERFACE
    hardware_uart
)
target_link_libraries(mlua_mod_hardware.uart INTERFACE
    hardware_base
    hardware_irq
    hardware_regs
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread_headers
    pico_platform
    pico_time
)

mlua_add_lua_modules(mlua_test_hardware.uart hardware.uart.test.lua)
target_link_libraries(mlua_test_hardware.uart INTERFACE
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.uart
    mlua_mod_math
    mlua_mod_mlua.list
    mlua_mod_mlua.testing.uart
    mlua_mod_mlua.thread
    mlua_mod_mlua.time
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware.vreg hardware.vreg.c)
target_link_libraries(mlua_mod_hardware.vreg INTERFACE
    hardware_vreg
)

mlua_add_lua_modules(mlua_test_hardware.vreg hardware.vreg.test.lua)
target_link_libraries(mlua_test_hardware.vreg INTERFACE
    mlua_mod_hardware.base
    mlua_mod_hardware.vreg
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.vreg_and_chip_reset
)

mlua_add_c_module(mlua_mod_hardware.watchdog hardware.watchdog.c)
target_link_libraries(mlua_mod_hardware.watchdog INTERFACE
    hardware_watchdog
)

mlua_add_lua_modules(mlua_test_hardware.watchdog hardware.watchdog.test.lua)
target_link_libraries(mlua_test_hardware.watchdog INTERFACE
    mlua_mod_hardware.base
    mlua_mod_hardware.regs.addressmap
    mlua_mod_hardware.regs.psm
    mlua_mod_hardware.regs.watchdog
    mlua_mod_hardware.watchdog
    mlua_mod_mlua.time
)

mlua_add_c_module(mlua_mod_hardware.xosc hardware.xosc.c)
target_link_libraries(mlua_mod_hardware.xosc INTERFACE
    hardware_xosc
)

mlua_add_lua_modules(mlua_test_hardware.xosc hardware.xosc.test.lua)
target_link_libraries(mlua_test_hardware.xosc INTERFACE
    mlua_mod_hardware.xosc
)

mlua_add_c_module(mlua_mod_lwip lwip.c)
target_compile_definitions(mlua_mod_lwip_headers INTERFACE
    CYW43_LWIP=1
)
target_include_directories(mlua_mod_lwip_headers INTERFACE
    include_lwip)
target_link_libraries(mlua_mod_lwip INTERFACE
    pico_lwip_core
    pico_lwip_nosys
)

mlua_add_lua_modules(mlua_test-net_lwip lwip.test.lua)
target_link_libraries(mlua_test-net_lwip INTERFACE
    mlua_mod_lwip
)

mlua_add_c_module(mlua_mod_lwip.dns lwip.dns.c)
target_compile_definitions(mlua_mod_lwip.dns_headers INTERFACE
    LWIP_DNS=1
    LWIP_UDP=1
)
target_link_libraries(mlua_mod_lwip.dns INTERFACE
    mlua_mod_lwip
    mlua_mod_mlua.thread
)

mlua_add_lua_modules(mlua_test-net_lwip.dns lwip.dns.test.lua)
target_link_libraries(mlua_test-net_lwip.dns INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.dns
    mlua_mod_mlua.time
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_lwip.ip4 lwip.ip4.c)
target_compile_definitions(mlua_mod_lwip.ip4_headers INTERFACE
    LWIP_IPV4=1
)
target_link_libraries(mlua_mod_lwip.ip4 INTERFACE
    mlua_mod_lwip
)

mlua_add_lua_modules(mlua_test-net_lwip.ip4 lwip.ip4.test.lua)
target_link_libraries(mlua_test-net_lwip.ip4 INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.ip4
    mlua_mod_lwip.netif
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_lwip.ip6 lwip.ip6.c)
target_compile_definitions(mlua_mod_lwip.ip6_headers INTERFACE
    LWIP_IPV6=1
)
target_link_libraries(mlua_mod_lwip.ip6 INTERFACE
    mlua_mod_lwip
)

mlua_add_lua_modules(mlua_test-net_lwip.ip6 lwip.ip6.test.lua)
target_link_libraries(mlua_test-net_lwip.ip6 INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.ip6
    mlua_mod_lwip.netif
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_lwip.netif lwip.netif.c)
target_link_libraries(mlua_mod_lwip.netif INTERFACE
    mlua_mod_lwip
)

mlua_add_lua_modules(mlua_test-net_lwip.netif lwip.netif.test.lua)
target_link_libraries(mlua_test-net_lwip.netif INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.ip4
    mlua_mod_lwip.ip6
    mlua_mod_lwip.netif
    mlua_mod_mlua.io
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_lwip.pbuf lwip.pbuf.c)
target_link_libraries(mlua_mod_lwip.pbuf INTERFACE
    mlua_mod_lwip
)

mlua_add_lua_modules(mlua_test-net_lwip.pbuf lwip.pbuf.test.lua)
target_link_libraries(mlua_test-net_lwip.pbuf INTERFACE
    mlua_mod_lwip.pbuf
    mlua_mod_mlua.mem
)

mlua_add_c_module(mlua_mod_lwip.stats lwip.stats.c)
target_compile_definitions(mlua_mod_lwip.stats_headers INTERFACE
    LWIP_STATS=1
)
target_link_libraries(mlua_mod_lwip.stats INTERFACE
    mlua_mod_lwip
)

mlua_add_lua_modules(mlua_test-net_lwip.stats lwip.stats.test.lua)
target_link_libraries(mlua_test-net_lwip.stats INTERFACE
    mlua_mod_lwip.stats
)

mlua_add_c_module(mlua_mod_lwip.tcp lwip.tcp.c)
target_compile_definitions(mlua_mod_lwip.tcp_headers INTERFACE
    LWIP_TCP=1
)
target_link_libraries(mlua_mod_lwip.tcp INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.pbuf
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread
)

mlua_add_lua_modules(mlua_test-net_lwip.tcp lwip.tcp.test.lua)
target_link_libraries(mlua_test-net_lwip.tcp INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.tcp
    mlua_mod_mlua.mem
    mlua_mod_mlua.testing.lwip
    mlua_mod_mlua.thread
    mlua_mod_mlua.thread.group
    mlua_mod_mlua.time
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_lwip.udp lwip.udp.c)
target_compile_definitions(mlua_mod_lwip.udp_headers INTERFACE
    LWIP_UDP=1
)
target_link_libraries(mlua_mod_lwip.udp INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.pbuf
    mlua_mod_mlua.thread
)

mlua_add_lua_modules(mlua_test-net_lwip.udp lwip.udp.test.lua)
target_link_libraries(mlua_test-net_lwip.udp INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.pbuf
    mlua_mod_lwip.udp
    mlua_mod_mlua.testing.lwip
    mlua_mod_mlua.thread
    mlua_mod_mlua.thread.group
    mlua_mod_mlua.time
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_mlua.block.flash mlua.block.flash.c)
target_include_directories(mlua_mod_mlua.block.flash_headers INTERFACE
    include_mlua.block.flash)
mlua_mirrored_target_link_libraries(mlua_mod_mlua.block.flash INTERFACE
    mlua_mod_mlua.block
)
target_link_libraries(mlua_mod_mlua.block.flash INTERFACE
    hardware_flash
    hardware_sync
    mlua_mod_mlua.errors
)

# TODO: Use a platform-specific library
target_link_libraries(mlua_mod_mlua.fs.lfs INTERFACE
    pico_sync
)

mlua_add_c_module(mlua_mod_mlua.fs.loader mlua.fs.loader.c)
target_compile_definitions(mlua_mod_mlua.fs.loader_headers INTERFACE
    LFS_THREADSAFE=1
)
target_link_libraries(mlua_mod_mlua.fs.loader INTERFACE
    hardware_flash
    mlua_mod_mlua.block.flash
    mlua_mod_mlua.errors
    mlua_mod_mlua.fs_headers
    mlua_mod_mlua.fs.lfs
    pico_binary_info
    pico_platform
)

mlua_add_lua_modules(mlua_test_mlua.fs.loader mlua.fs.loader.test.lua)
target_link_libraries(mlua_test_mlua.fs.loader INTERFACE
    mlua_mod_mlua.fs
    mlua_mod_mlua.fs.loader
    mlua_mod_mlua.util
    mlua_mod_package
    mlua_mod_string
)

mlua_add_lua_modules(mlua_test_mlua.stdio mlua.stdio.test.lua)
target_link_libraries(mlua_test_mlua.stdio INTERFACE
    mlua_mod_mlua.io
    mlua_mod_mlua.list
    mlua_mod_mlua.repr
    mlua_mod_mlua.stdio
    mlua_mod_mlua.testing.stdio
)

mlua_add_lua_modules(mlua_mod_mlua.testing.clocks mlua.testing.clocks.lua)
target_link_libraries(mlua_mod_mlua.testing.clocks INTERFACE
    mlua_mod_hardware.clocks
    mlua_mod_hardware.uart
    mlua_mod_pico.stdio
)

mlua_add_lua_modules(mlua_mod_mlua.testing.i2c mlua.testing.i2c.lua)
target_link_libraries(mlua_mod_mlua.testing.i2c INTERFACE
    mlua_mod_hardware.gpio
    mlua_mod_hardware.i2c
    mlua_mod_mlua.list
    mlua_mod_mlua.time
)

mlua_add_lua_modules(mlua_mod_mlua.testing.lwip mlua.testing.lwip.lua)
target_link_libraries(mlua_mod_mlua.testing.lwip INTERFACE
    mlua_mod_lwip
    mlua_mod_lwip.dns
    mlua_mod_lwip.stats
    mlua_mod_lwip.tcp
    mlua_mod_mlua.io
    mlua_mod_mlua.mem
    mlua_mod_mlua.oo
    mlua_mod_mlua.testing
    mlua_mod_mlua.time
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua.testing.platform mlua.testing.platform.lua)
target_link_libraries(mlua_mod_mlua.testing.platform INTERFACE
    mlua_mod_mlua.io
    mlua_mod_mlua.mem
    mlua_mod_mlua.testing
    mlua_mod_pico
    mlua_mod_pico.standard_link
)
target_link_libraries(mlua_mod_mlua.testing INTERFACE
    mlua_mod_mlua.testing.platform
)

mlua_add_lua_modules(mlua_mod_mlua.testing.stdio mlua.testing.stdio.lua)
target_link_libraries(mlua_mod_mlua.testing.stdio INTERFACE
    mlua_mod_hardware.gpio
    mlua_mod_hardware.uart
    mlua_mod_mlua.testing.uart
    mlua_mod_pico.stdio
    mlua_mod_pico.stdio.uart
)

mlua_add_lua_modules(mlua_mod_mlua.testing.uart mlua.testing.uart.lua)
target_link_libraries(mlua_mod_mlua.testing.uart INTERFACE
    mlua_mod_hardware.gpio
    mlua_mod_hardware.uart
)

target_include_directories(mlua_mod_mlua.thread_headers INTERFACE
    include_mlua.thread)
target_sources(mlua_mod_mlua.thread INTERFACE event.c)
target_link_libraries(mlua_mod_mlua.thread INTERFACE
    hardware_irq
    hardware_sync
)

mlua_add_c_module(mlua_mod_pico pico.c)
target_link_libraries(mlua_mod_pico INTERFACE
    pico_base
)

mlua_add_lua_modules(mlua_test_pico pico.test.lua)
target_link_libraries(mlua_test_pico INTERFACE
    mlua_mod_pico
)

mlua_add_header_module(mlua_mod_pico.board pico.board
    "${CMAKE_BINARY_DIR}/generated/pico_base/pico/config_autogen.h")
target_link_libraries(mlua_mod_pico.board INTERFACE
    pico_base
)

mlua_add_lua_modules(mlua_test_pico.board pico.board.test.lua)
target_link_libraries(mlua_test_pico.board INTERFACE
    mlua_mod_pico.board
)

mlua_add_c_module(mlua_mod_pico.bootrom pico.bootrom.c)
target_link_libraries(mlua_mod_pico.bootrom INTERFACE
    pico_bootrom
)

mlua_add_lua_modules(mlua_test_pico.bootrom pico.bootrom.test.lua)
target_link_libraries(mlua_test_pico.bootrom INTERFACE
    mlua_mod_mlua.list
    mlua_mod_mlua.mem
    mlua_mod_pico.bootrom
)

mlua_add_c_module(mlua_mod_pico.cyw43 pico.cyw43.c)
target_compile_definitions(mlua_mod_pico.cyw43_headers INTERFACE
    CYW43_CONFIG_FILE="mlua/cyw43_configport.h"
)
target_include_directories(mlua_mod_pico.cyw43_headers INTERFACE
    include_pico.cyw43)
target_link_libraries(mlua_mod_pico.cyw43 INTERFACE
    cyw43_driver_picow
    pico_cyw43_driver
    pico_unique_id
)

mlua_add_lua_modules(mlua_mod_pico.cyw43.util pico.cyw43.util.lua)
target_link_libraries(mlua_mod_pico.cyw43.util INTERFACE
    mlua_mod_mlua.time
    mlua_mod_pico
    mlua_mod_pico.cyw43
    mlua_mod_pico.cyw43.wifi
)

mlua_add_c_module(mlua_mod_pico.cyw43.wifi pico.cyw43.wifi.c)
target_link_libraries(mlua_mod_pico.cyw43.wifi INTERFACE
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread
    mlua_mod_pico.cyw43
)

mlua_add_c_module(mlua_mod_pico.i2c_slave pico.i2c_slave.c)
target_link_libraries(mlua_mod_pico.i2c_slave INTERFACE
    hardware_i2c
    mlua_mod_hardware.i2c
    mlua_mod_mlua.thread
)

mlua_add_lua_modules(mlua_test_pico.i2c_slave pico.i2c_slave.test.lua)
target_link_libraries(mlua_test_pico.i2c_slave INTERFACE
    mlua_mod_mlua.testing.i2c
    mlua_mod_pico.i2c_slave
)

mlua_add_c_module(mlua_mod_pico.multicore pico.multicore.c)
target_link_libraries(mlua_mod_pico.multicore INTERFACE
    mlua_mod_mlua.thread
    pico_multicore
    pico_platform
)

mlua_add_lua_modules(mlua_test_pico.multicore pico.multicore.test.lua)
target_link_libraries(mlua_test_pico.multicore INTERFACE
    mlua_mod_mlua.thread
    mlua_mod_mlua.time
    mlua_mod_pico.multicore
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_pico.multicore.fifo pico.multicore.fifo.c)
target_link_libraries(mlua_mod_pico.multicore.fifo INTERFACE
    hardware_structs
    hardware_sync
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread_headers
    pico_multicore
    pico_platform
    pico_time
)

mlua_add_lua_modules(mlua_test_pico.multicore.fifo pico.multicore.fifo.test.lua)
target_link_libraries(mlua_test_pico.multicore.fifo INTERFACE
    mlua_mod_mlua.thread
    mlua_mod_pico.multicore
    mlua_mod_pico.multicore.fifo
)

mlua_add_c_module(mlua_mod_pico.platform pico.platform.c)
target_link_libraries(mlua_mod_pico.platform INTERFACE
    pico_platform
)

mlua_add_lua_modules(mlua_test_pico.platform pico.platform.test.lua)
target_link_libraries(mlua_test_pico.platform INTERFACE
    mlua_mod_hardware.clocks
    mlua_mod_math
    mlua_mod_mlua.time
    mlua_mod_pico.platform
)

mlua_add_c_module(mlua_mod_pico.rand pico.rand.c)
target_link_libraries(mlua_mod_pico.rand INTERFACE
    mlua_mod_mlua.int64
    pico_rand
)

mlua_add_lua_modules(mlua_test_pico.rand pico.rand.test.lua)
target_link_libraries(mlua_test_pico.rand INTERFACE
    mlua_mod_mlua.int64
    mlua_mod_pico.rand
)

mlua_add_c_module(mlua_mod_pico.standard_link pico.standard_link.c)
target_link_libraries(mlua_mod_pico.standard_link INTERFACE
    pico_base
)

mlua_add_lua_modules(mlua_test-ALL_pico.standard_link
    pico.standard_link.test.lua)
target_link_libraries(mlua_test-ALL_pico.standard_link INTERFACE
    mlua_mod_pico.standard_link
)

mlua_add_c_module(mlua_mod_pico.stdio pico.stdio.c)
target_link_libraries(mlua_mod_pico.stdio INTERFACE
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread_headers
    pico_stdio
    pico_time
)

mlua_add_lua_modules(mlua_test_pico.stdio pico.stdio.test.lua)
target_link_libraries(mlua_test_pico.stdio INTERFACE
    mlua_mod_mlua.list
    mlua_mod_mlua.repr
    mlua_mod_mlua.testing.stdio
    mlua_mod_mlua.thread
    mlua_mod_pico
    mlua_mod_pico.stdio
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_pico.stdio.semihosting pico.stdio.semihosting.c)
target_link_libraries(mlua_mod_pico.stdio.semihosting INTERFACE
    hardware_exception
    mlua_mod_pico.stdio  # For convenience
    pico_stdio_semihosting
)

mlua_add_lua_modules(mlua_test_pico.stdio.semihosting
    pico.stdio.semihosting.test.lua)
target_link_libraries(mlua_test_pico.stdio.semihosting INTERFACE
    mlua_mod_pico.stdio
    mlua_mod_pico.stdio.semihosting
)

mlua_add_c_module(mlua_mod_pico.stdio.uart pico.stdio.uart.c)
target_link_libraries(mlua_mod_pico.stdio.uart INTERFACE
    mlua_mod_hardware.uart_headers
    mlua_mod_pico.stdio  # For convenience
    pico_stdio_uart
)

mlua_add_lua_modules(mlua_test_pico.stdio.uart pico.stdio.uart.test.lua)
target_link_libraries(mlua_test_pico.stdio.uart INTERFACE
    mlua_mod_hardware.gpio
    mlua_mod_hardware.uart
    mlua_mod_mlua.list
    mlua_mod_mlua.testing.uart
    mlua_mod_pico
    mlua_mod_pico.stdio.uart
)

mlua_add_c_module(mlua_mod_pico.stdio.usb pico.stdio.usb.c)
target_link_libraries(mlua_mod_pico.stdio.usb INTERFACE
    pico_stdio_usb
    mlua_mod_pico.stdio  # For convenience
)

mlua_add_lua_modules(mlua_test_pico.stdio.usb pico.stdio.usb.test.lua)
target_link_libraries(mlua_test_pico.stdio.usb INTERFACE
    mlua_mod_pico.stdio
    mlua_mod_pico.stdio.usb
)

mlua_add_c_module(mlua_mod_pico.stdlib pico.stdlib.c)
target_link_libraries(mlua_mod_pico.stdlib INTERFACE
    pico_stdlib
)

mlua_add_c_module(mlua_mod_pico.time pico.time.c)
target_link_libraries(mlua_mod_pico.time INTERFACE
    hardware_timer
    mlua_mod_mlua.int64
    mlua_mod_mlua.thread_headers
    pico_time
)

mlua_add_lua_modules(mlua_test_pico.time pico.time.test.lua)
target_link_libraries(mlua_test_pico.time INTERFACE
    mlua_mod_math
    mlua_mod_mlua.int64
    mlua_mod_mlua.list
    mlua_mod_mlua.thread
    mlua_mod_pico.time
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_pico.unique_id pico.unique_id.c)
target_link_libraries(mlua_mod_pico.unique_id INTERFACE
    pico_unique_id
)

mlua_add_lua_modules(mlua_test_pico.unique_id pico.unique_id.test.lua)
target_link_libraries(mlua_test_pico.unique_id INTERFACE
    mlua_mod_pico.unique_id
    mlua_mod_string
)
