mlua_add_lua_module(mlua_examples_demo_main
    main ${CMAKE_CURRENT_LIST_DIR}/main.lua)

add_executable(mlua_examples_demo)
target_link_libraries(mlua_examples_demo
    mlua_core_lib_all
    mlua_examples_demo_main
    mlua_lib_int64
    mlua_lib_module
    pico_stdlib
)

pico_add_extra_outputs(mlua_examples_demo)
pico_enable_stdio_uart(mlua_examples_demo 1)
pico_enable_stdio_usb(mlua_examples_demo 0)
