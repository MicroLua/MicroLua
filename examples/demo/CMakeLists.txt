add_executable(mlua_example_demo)
mlua_sources(mlua_example_demo
    ${CMAKE_CURRENT_LIST_DIR}/main.lua
)
target_link_libraries(mlua_example_demo
    pico_stdlib
    mlua_main
)

pico_add_extra_outputs(mlua_example_demo)
pico_enable_stdio_uart(mlua_example_demo 1)
pico_enable_stdio_usb(mlua_example_demo 0)
