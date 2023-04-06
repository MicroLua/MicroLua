#define LUA_INT_DEFAULT     LUA_INT_INT
// #define LUA_INT_DEFAULT     LUA_INT_LONGLONG
#define LUA_FLOAT_DEFAULT   LUA_FLOAT_FLOAT
// #define LUA_FLOAT_DEFAULT   LUA_FLOAT_DOUBLE

#define LUA_PATH_DEFAULT ""
#define LUA_CPATH_DEFAULT ""

#define LUA_NOCVTN2S
#define LUA_NOCVTS2N

@LUACONF@

#undef LUAI_MAXSTACK
#define LUAI_MAXSTACK       1000

// Lua uses the C99 %a printf specifier in lua_number2strx. The "pico" printf
// implementation (see pico_set_printf_implementation) doesn't support %a. The
// "compiler" implementation uses newlib, which does support %a but only if it
// is compiled with --enable-newlib-io-c99-formats. Lua provides a fallback if
// lua_number2strx is undefined.
#undef lua_number2strx
