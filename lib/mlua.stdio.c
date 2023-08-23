#include <unistd.h>

#include "pico/stdio.h"
#if LIB_PICO_STDIO_UART
#include "pico/stdio_uart.h"
#endif

#include "lua.h"
#include "lauxlib.h"
#include "mlua/pico.stdio.h"
#include "mlua/util.h"

// Silence link-time warnings.
__attribute__((weak)) int _link(char const* old, char const* new) { return -1; }
__attribute__((weak)) int _unlink(char const* file) { return -1; }

static char const InStream_name[] = "mlua.stdio.InStream";

static int InStream_read(lua_State* ls) {
    int fd = *((int*)luaL_checkudata(ls, 1, InStream_name));
#if LIB_MLUA_MOD_PICO_STDIO
    return mlua_pico_stdio_read(ls, fd, 2);
#else
    lua_Integer len = luaL_checkinteger(ls, 2);
    luaL_argcheck(ls, 0 <= len, 2, "invalid length");
    luaL_Buffer buf;
    char* p = luaL_buffinitsize(ls, &buf, len);
    int cnt = read(fd, p, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    luaL_pushresultsize(&buf, cnt);
    return 1;
#endif
}

static MLuaSym const InStream_syms[] = {
    MLUA_SYM_F(read, InStream_),
};

static char const OutStream_name[] = "mlua.stdio.OutStream";

static int OutStream_write(lua_State* ls) {
    int fd = *((int*)luaL_checkudata(ls, 1, OutStream_name));
#if LIB_MLUA_MOD_PICO_STDIO
    return mlua_pico_stdio_write(ls, fd, 2);
#else
    size_t len;
    char const* s = luaL_checklstring(ls, 2, &len);
    int cnt = write(fd, s, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    lua_pushinteger(ls, cnt);
    return 1;
#endif
}

static MLuaSym const OutStream_syms[] = {
    MLUA_SYM_F(write, OutStream_),
};

static void create_stream(lua_State* ls, char const* name, char const* cls,
                          int stream) {
    int mod = lua_gettop(ls);
    int* v = lua_newuserdatauv(ls, sizeof(int), 0);
    *v = stream;
    luaL_getmetatable(ls, cls);
    lua_setmetatable(ls, -2);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, mod, name);
    lua_setglobal(ls, name);
}

static void write_stdout(lua_State* ls, char const* s) {
    lua_getglobal(ls, "stdout");
    lua_getfield(ls, -1, "write");
    lua_rotate(ls, -2, 1);
    if (s != NULL) { lua_pushstring(ls, s); } else { lua_rotate(ls, -3, -1); }
    lua_call(ls, 2, 0);
}

static int global_print(lua_State* ls) {
    int top = lua_gettop(ls);
    for (int i = 1; i <= top; ++i) {
        if (i > 1) write_stdout(ls, "\t");
        luaL_tolstring(ls, i, NULL);
        write_stdout(ls, NULL);
    }
    write_stdout(ls, "\n");
    return 0;
}

static MLuaSym const module_syms[] = {
    MLUA_SYM_V(stderr, boolean, false),
    MLUA_SYM_V(stdin, boolean, false),
    MLUA_SYM_V(stdout, boolean, false),
};

// TODO: Allow customizing which stdio drivers to enable early

static __attribute__((constructor)) void init(void) {
#if LIB_PICO_STDIO_UART
    stdio_uart_init();
#endif
}

int luaopen_mlua_stdio(lua_State* ls) {
#if LIB_MLUA_MOD_PICO_STDIO
    mlua_require(ls, "pico.stdio", false);
#endif

    mlua_new_module(ls, 0, module_syms);

    // Create the InStream and OutStream classes.
    mlua_new_class(ls, InStream_name, InStream_syms);
    mlua_new_class(ls, OutStream_name, OutStream_syms);

    // Create objects for stdin, stdout and stderr. Set them in _G, too.
    create_stream(ls, "stdin", InStream_name, STDIN_FILENO);
    create_stream(ls, "stdout", OutStream_name, STDOUT_FILENO);
    create_stream(ls, "stderr", OutStream_name, STDERR_FILENO);

    // Override the print function.
    lua_pushcfunction(ls, global_print);
    lua_setglobal(ls, "print");
    return 1;
}