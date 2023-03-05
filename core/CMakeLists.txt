
include(core.cmake)

# Copy Lua headers except luaconf.h.
mlua_core_filenames(lua_h *.h)
list(FILTER lua_h EXCLUDE REGEX ^luaconf\\.h$)
mlua_core_copy("${lua_h}" include)

# Copy Lua sources.
mlua_core_filenames(lua_c *.c)
mlua_core_copy("${lua_c}" src)

# Generate luaconf.h.
# TODO: Allow overriding settings
mlua_core_copy(luaconf.h include)

# Core Lua interpreter.
mlua_core_library(mlua_core)
mlua_core_target_sources(mlua_core
    # CORE_O
    lapi.c
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
    # AUX_O
    lauxlib.c
)

# Core Lua libraries.
mlua_core_lib_library(base lbaselib.c)
mlua_core_lib_library(coro lcorolib.c)
mlua_core_lib_library(debug ldblib.c)
mlua_core_lib_library(io liolib.c)
mlua_core_lib_library(math lmathlib.c)
mlua_core_lib_library(os loslib.c)
mlua_core_lib_library(package loadlib.c)
mlua_core_lib_library(table ltablib.c)
mlua_core_lib_library(string lstrlib.c)
mlua_core_lib_library(utf8 lutf8lib.c)

# All core Lua libraries.
mlua_core_library(mlua_core_lib_all)
mlua_core_target_sources(mlua_core_lib_all
    # LIB_O
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
    mlua_core_lib_table
    mlua_core_lib_string
    mlua_core_lib_utf8
)

# A test program.
add_executable(mlua_test
    test_main.c
)

target_link_libraries(mlua_test
    pico_stdlib
    mlua_core
    mlua_core_lib_all
)

pico_add_extra_outputs(mlua_test)
pico_enable_stdio_uart(mlua_test 1)
pico_enable_stdio_usb(mlua_test 0)
