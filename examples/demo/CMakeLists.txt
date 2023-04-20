mlua_add_lua_modules(mlua_examples_demo_main main.lua)
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
