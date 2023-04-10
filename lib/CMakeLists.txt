mlua_add_c_module(mlua_mod_hardware_adc hardware.adc hardware.adc.c)
target_link_libraries(mlua_mod_hardware_adc INTERFACE
    hardware_adc
)

mlua_add_c_module(mlua_mod_hardware_gpio hardware.gpio hardware.gpio.c)
target_link_libraries(mlua_mod_hardware_gpio INTERFACE
    hardware_gpio
)

mlua_add_c_module(mlua_mod_int64 int64 int64.c)
target_include_directories(mlua_mod_int64_headers INTERFACE include_int64)

mlua_add_lua_test_module(mlua_mod_int64_tests int64.tests int64.tests.lua)
target_link_libraries(mlua_mod_int64_tests INTERFACE
    mlua_mod_int64
    mlua_mod_math
    mlua_mod_module
    mlua_mod_string
    mlua_mod_table
    mlua_mod_testing
)

mlua_add_lua_module(mlua_mod_module module module.lua)
target_link_libraries(mlua_mod_module INTERFACE
    mlua_mod_package
)

mlua_add_lua_module(mlua_mod_oo oo oo.lua)
target_link_libraries(mlua_mod_oo INTERFACE
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
    mlua_mod_int64
    pico_time
)

mlua_add_lua_module(mlua_mod_testing testing testing.lua)
target_link_libraries(mlua_mod_testing INTERFACE
    mlua_mod_io
    mlua_mod_module
    mlua_mod_oo
    mlua_mod_package
    mlua_mod_string
    mlua_mod_table
)
