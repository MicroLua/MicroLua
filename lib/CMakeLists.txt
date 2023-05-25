mlua_add_c_module(mlua_mod_hardware_adc hardware.adc hardware.adc.c)
target_link_libraries(mlua_mod_hardware_adc INTERFACE
    hardware_adc
    mlua_mod_hardware_irq
)

mlua_add_c_module(mlua_mod_hardware_gpio hardware.gpio hardware.gpio.c)
target_link_libraries(mlua_mod_hardware_gpio INTERFACE
    hardware_gpio
    hardware_structs
    hardware_sync
    mlua_mod_signal
    pico_platform
)

mlua_add_c_module(mlua_mod_hardware_irq hardware.irq hardware.irq.c)
target_include_directories(mlua_mod_hardware_irq_headers INTERFACE
    include_hardware.irq)
target_link_libraries(mlua_mod_hardware_irq INTERFACE
    hardware_irq
    hardware_sync
    mlua_mod_signal
    pico_platform
)

mlua_add_c_module(mlua_mod_hardware_timer hardware.timer hardware.timer.c)
target_link_libraries(mlua_mod_hardware_timer INTERFACE
    hardware_timer
    hardware_sync
    mlua_mod_mlua_int64
    mlua_mod_signal
    pico_platform
)

mlua_add_lua_modules(mlua_mod_mlua_eio mlua.eio.lua)
target_link_libraries(mlua_mod_mlua_eio INTERFACE
    mlua_mod_mlua_oo
    mlua_mod_module
    mlua_mod_string
)

mlua_add_c_module(mlua_mod_mlua_int64 mlua.int64 mlua.int64.c)
target_include_directories(mlua_mod_mlua_int64_headers INTERFACE
    include_mlua.int64)

mlua_add_lua_test_modules(mlua_mod_mlua_int64_tests mlua.int64.tests.lua)
target_link_libraries(mlua_mod_mlua_int64_tests INTERFACE
    mlua_mod_math
    mlua_mod_mlua_eio
    mlua_mod_mlua_int64
    mlua_mod_module
    mlua_mod_string
    mlua_mod_table
    mlua_mod_util
)

mlua_add_lua_modules(mlua_mod_module module.lua)
target_link_libraries(mlua_mod_module INTERFACE
    mlua_mod_package
)

mlua_add_lua_modules(mlua_mod_mlua_oo mlua.oo.lua)
target_link_libraries(mlua_mod_mlua_oo INTERFACE
    mlua_mod_module
)

mlua_add_c_module(mlua_mod_pico pico pico.c)
target_link_libraries(mlua_mod_pico INTERFACE
    pico_base
)

mlua_add_c_module(mlua_mod_pico_platform pico.platform pico.platform.c)
target_link_libraries(mlua_mod_pico_platform INTERFACE
    pico_platform
)

mlua_add_c_module(mlua_mod_pico_time pico.time pico.time.c)
target_link_libraries(mlua_mod_pico_time INTERFACE
    mlua_mod_mlua_int64
    pico_time
)

mlua_add_c_module(mlua_mod_signal signal signal.c)
target_include_directories(mlua_mod_signal_headers INTERFACE include_signal)
target_link_libraries(mlua_mod_signal INTERFACE
    hardware_sync
    pico_platform
)

mlua_add_lua_modules(mlua_mod_testing testing.lua)
target_link_libraries(mlua_mod_testing INTERFACE
    mlua_mod_mlua_eio
    mlua_mod_mlua_oo
    mlua_mod_module
    mlua_mod_os
    mlua_mod_package
    mlua_mod_string
    mlua_mod_table
)

mlua_add_lua_modules(mlua_mod_thread thread.lua)
target_link_libraries(mlua_mod_thread INTERFACE
    mlua_mod_coroutine
    mlua_mod_module
    mlua_mod_signal
    mlua_mod_string
)

add_executable(mlua_tests)
set(MLUA_TEST_TARGET_FILTER "${MLUA_TEST_TARGET_FILTER}" CACHE STRING
    "Test target filter regexp"
)
get_property(TESTS GLOBAL PROPERTY mlua_test_targets)
if(NOT "${MLUA_TEST_TARGET_FILTER}" STREQUAL "")
    list(FILTER TESTS INCLUDE REGEX "${MLUA_TEST_TARGET_FILTER}")
endif()
target_link_libraries(mlua_tests
    mlua_mod_testing
    pico_stdlib
    ${TESTS}
)
mlua_main(mlua_tests testing)
pico_add_extra_outputs(mlua_tests)

mlua_add_lua_modules(mlua_mod_util util.lua)
target_link_libraries(mlua_mod_util INTERFACE
    mlua_mod_module
    mlua_mod_table
)
