// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PLATFORM_LUACONF_H
#define _MLUA_LIB_PICO_PLATFORM_LUACONF_H

// Lua uses the C99 %a printf specifier in lua_number2strx. The "pico" printf
// implementation (see pico_set_printf_implementation) doesn't support %a. The
// "compiler" implementation uses newlib, which does support %a but only if it
// is compiled with --enable-newlib-io-c99-formats. Lua provides a fallback if
// lua_number2strx is undefined, so we use that.
#define MLUA_UNDEF_lua_number2strx 1

#define MLUA_MAXSTACK_DEFAULT 1000
#define MLUA_BUFFERSIZE_DEFAULT 32

#endif
