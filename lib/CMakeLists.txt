mlua_add_c_module(mlua_lib_hardware_adc hardware.adc hardware.adc.c)
target_link_libraries(mlua_lib_hardware_adc INTERFACE
    hardware_adc
)

mlua_add_c_module(mlua_lib_hardware_gpio hardware.gpio hardware.gpio.c)
target_link_libraries(mlua_lib_hardware_gpio INTERFACE
    hardware_gpio
)

mlua_add_c_module(mlua_lib_int64 int64 int64.c)
target_include_directories(mlua_lib_int64_headers INTERFACE include_int64)

mlua_add_lua_test_module(mlua_lib_int64_tests int64.tests int64.tests.lua)
target_link_libraries(mlua_lib_int64_tests INTERFACE
    mlua_lib_int64
    mlua_lib_module
)

mlua_add_lua_module(mlua_lib_module module module.lua)
target_link_libraries(mlua_lib_module INTERFACE
    mlua_core_lib_package
)

mlua_add_lua_module(mlua_lib_oo oo oo.lua)
target_link_libraries(mlua_lib_oo INTERFACE
    mlua_lib_module
)

mlua_add_c_module(mlua_lib_pico pico pico.c)

mlua_add_c_module(mlua_lib_pico_time pico.time pico.time.c)
target_link_libraries(mlua_lib_pico_time INTERFACE
    pico_time
)

mlua_add_lua_module(mlua_lib_testing testing testing.lua)
target_link_libraries(mlua_lib_testing INTERFACE
    mlua_core_lib_io
    mlua_core_lib_package
    mlua_core_lib_string
    mlua_core_lib_table
    mlua_lib_module
    mlua_lib_oo
)
