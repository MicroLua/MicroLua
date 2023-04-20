mlua_add_lua_module(mlua_examples_adc_onboard_temperature_main
    main onboard_temperature_main.lua
)
target_link_libraries(mlua_examples_adc_onboard_temperature_main INTERFACE
    mlua_mod_hardware_adc
    mlua_mod_hardware_gpio
    mlua_mod_module
    mlua_mod_pico
    mlua_mod_pico_time
    mlua_mod_string
)

add_executable(mlua_examples_adc_onboard_temperature)
target_link_libraries(mlua_examples_adc_onboard_temperature
    mlua_examples_adc_onboard_temperature_main
    pico_stdlib
)
pico_add_extra_outputs(mlua_examples_adc_onboard_temperature)
