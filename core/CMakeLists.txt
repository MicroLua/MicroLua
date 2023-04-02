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
mlua_add_core_library(mlua_core_lib_all
    linit.c
)
target_link_libraries(mlua_core_lib_all INTERFACE
    mlua_core_lib_base
    mlua_core_lib_coroutine
    mlua_core_lib_debug
    mlua_core_lib_io
    mlua_core_lib_math
    mlua_core_lib_os
    mlua_core_lib_package
    mlua_core_lib_string
    mlua_core_lib_table
    mlua_core_lib_utf8
)

# The entry point of the Lua interpreter.
pico_add_library(mlua_main)
target_sources(mlua_main INTERFACE
    lib.c
    main.c
)
target_include_directories(mlua_main INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(mlua_main INTERFACE
    mlua_core
    mlua_core_lib_base
    mlua_core_lib_package
)
