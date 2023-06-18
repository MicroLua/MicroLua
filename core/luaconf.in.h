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

// Lua uses the C99 %a printf specifier in lua_number2strx. The "pico" printf
// implementation (see pico_set_printf_implementation) doesn't support %a. The
// "compiler" implementation uses newlib, which does support %a but only if it
// is compiled with --enable-newlib-io-c99-formats. Lua provides a fallback if
// lua_number2strx is undefined, so we use that.
#undef lua_number2strx

#ifdef PICO_BUILD

#if @MLUA_MAXSTACK@ > 0
#undef LUAI_MAXSTACK
#define LUAI_MAXSTACK @MLUA_MAXSTACK@
#endif

#if @MLUA_BUFFERSIZE@ > 0
#undef LUAL_BUFFERSIZE
#define LUAL_BUFFERSIZE @MLUA_BUFFERSIZE@
#endif

// lua_writestring and lua_writeline are only used by print(), which we override
// in main.c.
#define lua_writestring(s, l) do { (void)s; (void)l; } while (0)
#define lua_writeline() do {} while (0)

// lua_writestringerror is used by panic(), warn() and debug.debug(). Use our
// own simplified implementation to avoid depending on sprintf.
#define lua_writestringerror(s, p) mlua_writestringerror(s, p)
extern void mlua_writestringerror(char const* fmt, char const* param);

#endif  // PICO_BUILD

#endif
