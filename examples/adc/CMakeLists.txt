mlua_add_lua_module(mlua_examples_adc_onboard_temperature_main
    main ${CMAKE_CURRENT_LIST_DIR}/onboard_temperature_main.lua)

add_executable(mlua_examples_adc_onboard_temperature)
target_link_libraries(mlua_examples_adc_onboard_temperature
    mlua_core_lib_string
    mlua_examples_adc_onboard_temperature_main
    mlua_lib_hardware.adc
    mlua_lib_hardware.gpio
    mlua_lib_module
    mlua_lib_pico
    mlua_lib_pico.time
    pico_stdlib
)

pico_add_extra_outputs(mlua_examples_adc_onboard_temperature)
pico_enable_stdio_uart(mlua_examples_adc_onboard_temperature 1)
pico_enable_stdio_usb(mlua_examples_adc_onboard_temperature 0)
