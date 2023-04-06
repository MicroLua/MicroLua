mlua_add_lua_module(mlua_tools_tests_main main tests_main.lua)
target_link_libraries(mlua_tools_tests_main INTERFACE
    mlua_mod_module
    mlua_mod_testing
)

add_executable(mlua_tools_tests)
get_property(tests GLOBAL PROPERTY mlua_test_targets)
target_link_libraries(mlua_tools_tests
    mlua_tools_tests_main
    pico_stdlib
    ${tests}
)

pico_add_extra_outputs(mlua_tools_tests)
pico_enable_stdio_uart(mlua_tools_tests 1)
pico_enable_stdio_usb(mlua_tools_tests 0)
