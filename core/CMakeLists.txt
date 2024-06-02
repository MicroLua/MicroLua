# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

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
)
target_link_libraries(mlua_core INTERFACE
    mlua_platform_luaconf
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

# The entry point of the Lua interpreter.
mlua_add_library(mlua_core_main INTERFACE)
target_compile_definitions(mlua_core_main INTERFACE
    _GNU_SOURCE
)
target_sources(mlua_core_main INTERFACE
    main.c
    module.c
    util.c
)
target_include_directories(mlua_core_main_headers INTERFACE include)
mlua_mirrored_target_link_libraries(mlua_core_main INTERFACE
    mlua_platform
)
target_link_libraries(mlua_core_main INTERFACE
    mlua_core
    mlua_mod_base
    mlua_mod_package
)
