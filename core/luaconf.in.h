#ifndef _MLUA_CORE_LUACONF_H
#define _MLUA_CORE_LUACONF_H

#define LUA_INT_DEFAULT LUA_INT_@MLUA_INT@
#define LUA_FLOAT_DEFAULT LUA_FLOAT_@MLUA_FLOAT@

#define LUA_PATH_DEFAULT ""
#define LUA_CPATH_DEFAULT ""

#if !@MLUA_NUM_TO_STR_CONV@
#define LUA_NOCVTN2S
#endif
#if !@MLUA_STR_TO_NUM_CONV@
#define LUA_NOCVTS2N
#endif

@LUACONF@

#undef LUAI_MAXSTACK
#define LUAI_MAXSTACK @MLUA_MAXSTACK@

// Lua uses the C99 %a printf specifier in lua_number2strx. The "pico" printf
// implementation (see pico_set_printf_implementation) doesn't support %a. The
// "compiler" implementation uses newlib, which does support %a but only if it
// is compiled with --enable-newlib-io-c99-formats. Lua provides a fallback if
// lua_number2strx is undefined.
#undef lua_number2strx

#endif
