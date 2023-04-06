mlua_add_lua_module(mlua_examples_adc_onboard_temperature_main
    main onboard_temperature_main.lua
)
target_link_libraries(mlua_examples_adc_onboard_temperature_main INTERFACE
    mlua_core_lib_string
    mlua_lib_hardware_adc
    mlua_lib_hardware_gpio
    mlua_lib_module
    mlua_lib_pico
    mlua_lib_pico_time
)

add_executable(mlua_examples_adc_onboard_temperature)
target_link_libraries(mlua_examples_adc_onboard_temperature
    mlua_examples_adc_onboard_temperature_main
    pico_stdlib
)

pico_add_extra_outputs(mlua_examples_adc_onboard_temperature)
pico_enable_stdio_uart(mlua_examples_adc_onboard_temperature 1)
pico_enable_stdio_usb(mlua_examples_adc_onboard_temperature 0)
