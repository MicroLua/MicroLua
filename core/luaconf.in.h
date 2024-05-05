// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_CORE_LUACONF_H
#define _MLUA_CORE_LUACONF_H

#include "mlua/platform_luaconf.h"

#define LUA_INT_DEFAULT LUA_INT_@MLUA_INT@
#define LUA_FLOAT_DEFAULT LUA_FLOAT_@MLUA_FLOAT@

#define LUA_PATH_DEFAULT ""
#define LUA_CPATH_DEFAULT ""

// When true, enable automatic coercion from number to string.
#if !MLUA_NUM_TO_STR_CONV
#define LUA_NOCVTN2S
#endif
// When true, enable automatic coercion from string to number.
#if !MLUA_STR_TO_NUM_CONV
#define LUA_NOCVTS2N
#endif

@LUACONF@

#if MLUA_UNDEF_lua_number2strx
#undef lua_number2strx
#endif

// The Lua stack size limit. When <= 0, use the default.
#ifndef MLUA_MAXSTACK
#define MLUA_MAXSTACK MLUA_MAXSTACK_DEFAULT
#endif
#if MLUA_MAXSTACK > 0
#undef LUAI_MAXSTACK
#define LUAI_MAXSTACK MLUA_MAXSTACK
#endif

// The size of the raw extra per-thread memory.
#ifdef MLUA_EXTRASPACE
#undef LUA_EXTRASPACE
#define LUA_EXTRASPACE MLUA_EXTRASPACE
#endif

// The initial buffer size used by lauxlib. When <= 0, use the default.
#ifndef MLUA_BUFFERSIZE
#define MLUA_BUFFERSIZE MLUA_BUFFERSIZE_DEFAULT
#endif
#if MLUA_BUFFERSIZE > 0
#undef LUAL_BUFFERSIZE
#define LUAL_BUFFERSIZE MLUA_BUFFERSIZE
#endif

// lua_writestring and lua_writeline are only used by print(), which we override
// in main.c.
#define lua_writestring(s, l) do { (void)s; (void)l; } while (0)
#define lua_writeline() do {} while (0)

// lua_writestringerror is used by panic(), warn() and debug.debug(). Use our
// own simplified implementation to avoid depending on sprintf.
#define lua_writestringerror(s, p) mlua_writestringerror(s, p)
void mlua_writestringerror(char const* fmt, ...);

#endif
