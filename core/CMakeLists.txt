# Copy Lua headers and generate luaconf.h.
mlua_core_filenames(lua_h *.h)
list(FILTER lua_h EXCLUDE REGEX ^luaconf\\.h$)
mlua_core_copy("${lua_h}" include)
mlua_core_luaconf(include)

# Copy Lua sources.
mlua_core_filenames(lua_c *.c)
mlua_core_copy("${lua_c}" src)

# Core Lua interpreter.
mlua_add_core_library(mlua_core
    lapi.c
    lauxlib.c
    lcode.c
    lctype.c
    ldebug.c
    ldo.c
    ldump.c
    lfunc.c
    lgc.c
    llex.c
    lmem.c
    lobject.c
    lopcodes.c
    lparser.c
    lstate.c
    lstring.c
    ltable.c
    ltm.c
    lundump.c
    lvm.c
    lzio.c
    ltests.c
)

# Core Lua modules.
mlua_add_core_c_module_noreg(base lbaselib.c)
mlua_add_core_c_module_noreg(package loadlib.c)
mlua_add_core_c_module(coroutine lcorolib.c)
mlua_add_core_c_module(debug ldblib.c)
mlua_add_core_c_module(io liolib.c)
mlua_add_core_c_module(math lmathlib.c)
mlua_add_core_c_module(os loslib.c)
mlua_add_core_c_module(string lstrlib.c)
mlua_add_core_c_module(table ltablib.c)
mlua_add_core_c_module(utf8 lutf8lib.c)

# All core Lua modules.
mlua_add_core_library(mlua_core_mod_all
    linit.c
)
target_link_libraries(mlua_core_mod_all INTERFACE
    mlua_mod_base
    mlua_mod_coroutine
    mlua_mod_debug
    mlua_mod_io
    mlua_mod_math
    mlua_mod_os
    mlua_mod_package
    mlua_mod_string
    mlua_mod_table
    mlua_mod_utf8
)

# The entry point of the Lua interpreter.
pico_add_library(mlua_core_main)
target_sources(mlua_core_main INTERFACE
    main.c
    module.c
    util.c
)
target_compile_definitions(mlua_core_main INTERFACE
    MLUA_MAIN_MODULE=$<IF:$<STREQUAL:$<TARGET_PROPERTY:MLUA_TARGET_MAIN_MODULE>,>,main,$<TARGET_PROPERTY:MLUA_TARGET_MAIN_MODULE>>
    MLUA_MAIN_FUNCTION=$<IF:$<STREQUAL:$<TARGET_PROPERTY:MLUA_TARGET_MAIN_FUNCTION>,>,main,$<TARGET_PROPERTY:MLUA_TARGET_MAIN_FUNCTION>>
)
target_include_directories(mlua_core_main INTERFACE
    "${CMAKE_CURRENT_LIST_DIR}/include"
)
target_link_libraries(mlua_core_main INTERFACE
    hardware_sync
    mlua_core
    mlua_mod_base
    mlua_mod_package
    pico_platform
)
