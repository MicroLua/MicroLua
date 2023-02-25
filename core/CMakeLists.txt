add_executable(mlua_test
    test_main.c
)

target_link_libraries(mlua_test
    pico_stdlib
)

pico_add_extra_outputs(mlua_test)
pico_enable_stdio_uart(mlua_test 1)
pico_enable_stdio_usb(mlua_test 0)
