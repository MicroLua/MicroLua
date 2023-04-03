mlua_add_lua_module(mlua_examples_blink_main
    main ${CMAKE_CURRENT_LIST_DIR}/main.lua)

add_executable(mlua_examples_blink)
target_link_libraries(mlua_examples_blink
    mlua_examples_blink_main
    mlua_lib_hardware.gpio
    mlua_lib_module
    mlua_lib_pico
    mlua_lib_pico.time
    pico_stdlib
)

pico_add_extra_outputs(mlua_examples_blink)
pico_enable_stdio_uart(mlua_examples_blink 1)
pico_enable_stdio_usb(mlua_examples_blink 0)
