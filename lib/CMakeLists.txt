mlua_add_c_module(mlua_mod_hardware_adc hardware.adc hardware.adc.c)
target_link_libraries(mlua_mod_hardware_adc INTERFACE
    hardware_adc
    hardware_irq
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_test_modules(mlua_mod_hardware_adc_test hardware.adc.test.lua)
target_link_libraries(mlua_mod_hardware_adc_test INTERFACE
    mlua_mod_hardware_adc
    mlua_mod_pico_time
)

mlua_add_c_module(mlua_mod_hardware_clocks hardware.clocks hardware.clocks.c)
target_link_libraries(mlua_mod_hardware_clocks INTERFACE
    hardware_clocks
)

mlua_add_lua_test_modules(mlua_mod_hardware_clocks_test
    hardware.clocks.test.lua)
target_link_libraries(mlua_mod_hardware_clocks_test INTERFACE
    mlua_mod_hardware_clocks
    mlua_mod_table
)

mlua_add_c_module(mlua_mod_hardware_gpio hardware.gpio hardware.gpio.c)
target_link_libraries(mlua_mod_hardware_gpio INTERFACE
    hardware_gpio
    hardware_structs
    hardware_sync
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_test_modules(mlua_mod_hardware_gpio_test hardware.gpio.test.lua)
target_link_libraries(mlua_mod_hardware_gpio_test INTERFACE
    mlua_mod_hardware_gpio
)

mlua_add_c_module(mlua_mod_hardware_irq hardware.irq hardware.irq.c)
target_link_libraries(mlua_mod_hardware_irq INTERFACE
    hardware_irq
    hardware_sync
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_test_modules(mlua_mod_hardware_irq_test hardware.irq.test.lua)
target_link_libraries(mlua_mod_hardware_irq_test INTERFACE
    mlua_mod_hardware_irq
    mlua_mod_mlua_thread
    mlua_mod_mlua_util
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_hardware_timer hardware.timer hardware.timer.c)
target_link_libraries(mlua_mod_hardware_timer INTERFACE
    hardware_timer
    mlua_mod_mlua_int64
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_test_modules(mlua_mod_hardware_timer_test hardware.timer.test.lua)
target_link_libraries(mlua_mod_hardware_timer_test INTERFACE
    mlua_mod_hardware_timer
)

mlua_add_c_module(mlua_mod_hardware_uart hardware.uart hardware.uart.c)
target_link_libraries(mlua_mod_hardware_uart INTERFACE
    hardware_irq
    hardware_regs
    hardware_uart
    mlua_mod_mlua_event_headers
    pico_platform
)

mlua_add_lua_test_modules(mlua_mod_hardware_uart_test hardware.uart.test.lua)
target_link_libraries(mlua_mod_hardware_uart_test INTERFACE
    mlua_mod_hardware_uart
    mlua_mod_mlua_thread
)

mlua_add_lua_modules(mlua_mod_mlua mlua.lua)
target_link_libraries(mlua_mod_mlua INTERFACE
    mlua_mod_package
)

mlua_add_lua_test_modules(mlua_mod_mlua_test mlua.test.lua)

mlua_add_c_module(mlua_mod_mlua_event mlua.event mlua.event.c)
target_include_directories(mlua_mod_mlua_event_headers INTERFACE
    include_mlua.event)
target_link_libraries(mlua_mod_mlua_event INTERFACE
    hardware_irq
    hardware_sync
    mlua_mod_mlua_int64
    pico_platform
    pico_time
)

mlua_add_c_module(mlua_mod_mlua_int64 mlua.int64 mlua.int64.c)
target_include_directories(mlua_mod_mlua_int64_headers INTERFACE
    include_mlua.int64)

mlua_add_lua_test_modules(mlua_mod_mlua_int64_test mlua.int64.test.lua)
target_link_libraries(mlua_mod_mlua_int64_test INTERFACE
    mlua_mod_math
    mlua_mod_mlua_int64
    mlua_mod_mlua_util
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_mlua_io mlua.io.lua)
target_link_libraries(mlua_mod_mlua_io INTERFACE
    mlua_mod_mlua_oo
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_test_modules(mlua_mod_mlua_io_test mlua.io.test.lua)
target_link_libraries(mlua_mod_mlua_io_test INTERFACE
    mlua_mod_mlua_io
)

mlua_add_lua_modules(mlua_mod_mlua_oo mlua.oo.lua)

mlua_add_lua_test_modules(mlua_mod_mlua_oo_test mlua.oo.test.lua)
target_link_libraries(mlua_mod_mlua_oo_test INTERFACE
    mlua_mod_mlua_oo
)

mlua_add_lua_modules(mlua_mod_mlua_testing mlua.testing.lua)
target_link_libraries(mlua_mod_mlua_testing INTERFACE
    mlua_mod_debug
    mlua_mod_mlua_io
    mlua_mod_mlua_oo
    mlua_mod_mlua_util
    mlua_mod_os
    mlua_mod_package
    mlua_mod_string
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

mlua_add_lua_test_modules(mlua_mod_mlua_thread_test mlua.thread.test.lua)
target_link_libraries(mlua_mod_mlua_thread_test INTERFACE
    mlua_mod_mlua_thread
    mlua_mod_string
)

mlua_add_lua_modules(mlua_mod_mlua_util mlua.util.lua)
target_link_libraries(mlua_mod_mlua_util INTERFACE
    mlua_mod_table
)

mlua_add_lua_test_modules(mlua_mod_mlua_util_test mlua.util.test.lua)
target_link_libraries(mlua_mod_mlua_util_test INTERFACE
    mlua_mod_mlua_util
)

mlua_add_c_module(mlua_mod_pico pico pico.c)
target_link_libraries(mlua_mod_pico INTERFACE
    pico_base
)

mlua_add_c_module(mlua_mod_pico_multicore pico.multicore pico.multicore.c)
target_link_libraries(mlua_mod_pico_multicore INTERFACE
    pico_multicore
)

mlua_add_lua_test_modules(mlua_mod_pico_multicore_test pico.multicore.test.lua)
target_link_libraries(mlua_mod_pico_multicore_test INTERFACE
    mlua_mod_pico_multicore
)

mlua_add_c_module(mlua_mod_pico_platform pico.platform pico.platform.c)
target_link_libraries(mlua_mod_pico_platform INTERFACE
    pico_platform
)

mlua_add_lua_test_modules(mlua_mod_pico_platform_test pico.platform.test.lua)
target_link_libraries(mlua_mod_pico_platform_test INTERFACE
    mlua_mod_pico_platform
)

mlua_add_c_module(mlua_mod_pico_stdio pico.stdio pico.stdio.c)
target_link_libraries(mlua_mod_pico_stdio INTERFACE
    mlua_mod_mlua_event_headers
    pico_stdio
)

mlua_add_lua_test_modules(mlua_mod_pico_stdio_test pico.stdio.test.lua)
target_link_libraries(mlua_mod_pico_stdio_test INTERFACE
    mlua_mod_pico_stdio
)

mlua_add_c_module(mlua_mod_pico_stdlib pico.stdlib pico.stdlib.c)
target_link_libraries(mlua_mod_pico_stdlib INTERFACE
    pico_stdlib
)

mlua_add_lua_test_modules(mlua_mod_pico_stdlib_test pico.stdlib.test.lua)
target_link_libraries(mlua_mod_pico_stdlib_test INTERFACE
    mlua_mod_pico_stdlib
)

mlua_add_c_module(mlua_mod_pico_time pico.time pico.time.c)
target_link_libraries(mlua_mod_pico_time INTERFACE
    hardware_timer
    mlua_mod_mlua_event_headers
    mlua_mod_mlua_int64
    pico_time
)

mlua_add_lua_test_modules(mlua_mod_pico_time_test pico.time.test.lua)
target_link_libraries(mlua_mod_pico_time_test INTERFACE
    mlua_mod_pico_time
)

mlua_add_c_module(mlua_mod_pico_unique_id pico.unique_id pico.unique_id.c)
target_link_libraries(mlua_mod_pico_unique_id INTERFACE
    pico_unique_id
)

mlua_add_lua_test_modules(mlua_mod_pico_unique_id_test pico.unique_id.test.lua)
target_link_libraries(mlua_mod_pico_unique_id_test INTERFACE
    mlua_mod_pico_unique_id
)

set(MLUA_TEST_TARGET_FILTER "${MLUA_TEST_TARGET_FILTER}" CACHE STRING
    "Test target filter regexp"
)
get_property(TESTS GLOBAL PROPERTY mlua_test_targets)
if(NOT "${MLUA_TEST_TARGET_FILTER}" STREQUAL "")
    list(FILTER TESTS INCLUDE REGEX "${MLUA_TEST_TARGET_FILTER}")
endif()

add_executable(mlua_tests)
target_link_libraries(mlua_tests
    mlua_mod_mlua_testing
    mlua_mod_mlua_thread
    pico_stdlib
    ${TESTS}
)
mlua_main(mlua_tests mlua.testing)
pico_add_extra_outputs(mlua_tests)
