mlua_add_lua_module(mlua_examples_demo_main main main.lua)
target_link_libraries(mlua_examples_demo_main INTERFACE
    mlua_core_mod_all
    mlua_mod_int64
    mlua_mod_module
    mlua_mod_pico
    mlua_mod_pico_platform
    mlua_mod_pico_time
)

add_executable(mlua_examples_demo)
target_link_libraries(mlua_examples_demo
    mlua_examples_demo_main
    pico_stdlib
)

pico_add_extra_outputs(mlua_examples_demo)
pico_enable_stdio_uart(mlua_examples_demo 1)
pico_enable_stdio_usb(mlua_examples_demo 0)
