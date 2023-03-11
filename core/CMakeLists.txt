include(core.cmake)

# Copy Lua headers except luaconf.h.
mlua_core_filenames(lua_h *.h)
list(FILTER lua_h EXCLUDE REGEX ^luaconf\\.h$)
mlua_core_copy("${lua_h}" include)

# Copy Lua sources.
mlua_core_filenames(lua_c *.c)
mlua_core_copy("${lua_c}" src)

# Generate luaconf.h.
# TODO: Allow overriding luaconf.h settings
mlua_core_copy(luaconf.h include)

# Core Lua interpreter.
mlua_core_library(mlua_core)
mlua_core_target_sources(mlua_core
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

# Core Lua libraries.
mlua_core_lib_library(base lbaselib.c)
mlua_core_lib_library(coro lcorolib.c)
mlua_core_lib_library(debug ldblib.c)
mlua_core_lib_library(io liolib.c)
mlua_core_lib_library(math lmathlib.c)
mlua_core_lib_library(os loslib.c)
mlua_core_lib_library(package loadlib.c)
mlua_core_lib_library(string lstrlib.c)
mlua_core_lib_library(table ltablib.c)
mlua_core_lib_library(utf8 lutf8lib.c)

# All core Lua libraries.
mlua_core_library(mlua_core_lib_all)
mlua_core_target_sources(mlua_core_lib_all
    linit.c
)
target_link_libraries(mlua_core_lib_all INTERFACE
    mlua_core_lib_base
    mlua_core_lib_coro
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
    file.c
    main.c
)
target_include_directories(mlua_main INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/include
)
target_link_libraries(mlua_main INTERFACE
    mlua_core
    mlua_core_lib_all
)
