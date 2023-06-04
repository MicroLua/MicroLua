mlua_add_lua_modules(mlua_examples_demo_main main.lua)
target_link_libraries(mlua_examples_demo_main INTERFACE
    mlua_core_mod_all
    mlua_examples_demo_main1
    mlua_mod_hardware_clocks
    mlua_mod_hardware_gpio
    mlua_mod_hardware_irq
    mlua_mod_hardware_timer
    mlua_mod_hardware_uart
    mlua_mod_mlua_eio
    mlua_mod_mlua_module
    mlua_mod_mlua_thread
    mlua_mod_pico
    mlua_mod_pico_multicore
    mlua_mod_pico_platform
    mlua_mod_pico_stdio
    mlua_mod_pico_stdlib
    mlua_mod_pico_time
    mlua_mod_pico_unique_id
)

mlua_add_lua_modules(mlua_examples_demo_main1 main1.lua)
target_link_libraries(mlua_examples_demo_main1 INTERFACE
    mlua_mod_mlua_eio
    mlua_mod_mlua_module
    mlua_mod_pico_time
)

add_executable(mlua_examples_demo)
target_link_libraries(mlua_examples_demo
    mlua_examples_demo_main
    pico_stdlib
)
pico_add_extra_outputs(mlua_examples_demo)
