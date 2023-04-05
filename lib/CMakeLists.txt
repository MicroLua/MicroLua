mlua_add_c_module(mlua_lib_hardware.adc hardware.adc hardware.adc.c)
target_link_libraries(mlua_lib_hardware.adc INTERFACE
    hardware_adc
)

mlua_add_c_module(mlua_lib_hardware.gpio hardware.gpio hardware.gpio.c)
target_link_libraries(mlua_lib_hardware.gpio INTERFACE
    hardware_gpio
)

mlua_add_c_module(mlua_lib_int64 int64 int64.c)
target_include_directories(mlua_lib_int64_headers INTERFACE include_int64)

mlua_add_lua_module(mlua_lib_module module module.lua)
target_link_libraries(mlua_lib_module INTERFACE
    mlua_core_lib_package
)

mlua_add_lua_module(mlua_lib_oo oo oo.lua)
target_link_libraries(mlua_lib_oo INTERFACE
    mlua_lib_module
)

mlua_add_c_module(mlua_lib_pico pico pico.c)

mlua_add_c_module(mlua_lib_pico.time pico.time pico.time.c)
target_link_libraries(mlua_lib_pico.time INTERFACE
    pico_time
)
