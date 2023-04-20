#include "mlua/main.h"

#include <stdio.h>

#include "lua.h"
#include "lauxlib.h"

#ifdef LIB_PICO_STDIO
#include "pico/stdio.h"
#endif

#include "mlua/lib.h"
#include "mlua/util.h"

static int getfield(lua_State* ls) {
    lua_gettable(ls, -2);
    return 1;
}

static int try_getfield(lua_State* ls, int index, char const* k) {
    index = lua_absindex(ls, index);
    lua_pushcfunction(ls, getfield);
    lua_pushvalue(ls, index);
    lua_pushstring(ls, k);
    return lua_pcall(ls, 2, 1, 0);
}

#ifdef LIB_PICO_STDIO

static int std_stream_read(lua_State* ls) {
    FILE** f = luaL_checkudata(ls, 1, "StdStream");
    lua_Integer len = luaL_optinteger(ls, 2, -1);
    if (*f != stdin) return luaL_error(ls, "read from non-input stream");
    luaL_Buffer buf;
    if (len >= 0) {  // Read at least one and at most len bytes
        char* p = luaL_buffinitsize(ls, &buf, len);
        int i = 0, c;
        while (i < len) {
            // There is no fgetc_timeout_us, but since there's only one input
            // stream, we can use getchar_timeout_us.
            c = i == 0 ? fgetc(*f) : getchar_timeout_us(10000);
            if (c == EOF || c == PICO_ERROR_TIMEOUT) break;
            p[i++] = c;
        }
        luaL_pushresultsize(&buf, i);
    } else {  // Read a line.
        luaL_buffinit(ls, &buf);
        int c;
        do {
            char* p = luaL_prepbuffer(&buf);
            int i = 0;
            do {
                c = fgetc(*f);
                if (c == EOF) break;
                p[i++] = c;
            } while (i < LUAL_BUFFERSIZE && c != '\n');
            luaL_addsize(&buf, i);
        } while (c != EOF && c != '\n');
        luaL_pushresult(&buf);
    }
    if (ferror(*f)) return luaL_fileresult(ls, 0, NULL);
    return 1;
}

static int std_stream_write(lua_State* ls) {
    FILE** f = luaL_checkudata(ls, 1, "StdStream");
    if (*f != stdout && *f != stderr)
        return luaL_error(ls, "write to non-output stream");
    int top = lua_gettop(ls);
    for (int i = 2; i <= top; ++i) {
        size_t len;
        char const* s = luaL_checklstring(ls, i, &len);
        if (fwrite(s, sizeof(char), len, *f) != len)
            return luaL_fileresult(ls, 0, NULL);
    }
    lua_pushvalue(ls, 1);
    return 1;
}

static int std_stream_flush(lua_State* ls) {
    FILE** f = luaL_checkudata(ls, 1, "StdStream");
    return luaL_fileresult(ls, fflush(*f) == 0, NULL);
}

static int std_stream_setvbuf(lua_State* ls) {
    FILE** f = luaL_checkudata(ls, 1, "StdStream");
    static char const* const mode_strs[] = {"no", "full", "line", NULL};
    int op = luaL_checkoption(ls, 2, NULL, mode_strs);
    lua_Integer size = luaL_optinteger(ls, 3, LUAL_BUFFERSIZE);
    static int const modes[] = {_IONBF, _IOFBF, _IOLBF};
    return luaL_fileresult(ls, setvbuf(*f, NULL, modes[op], size) == 0, NULL);
}

static void std_write(lua_State* ls, int index, char const* s, size_t len) {
    index = lua_absindex(ls, index);
    lua_pushcfunction(ls, std_stream_write);
    lua_pushvalue(ls, index);
    lua_pushlstring(ls, s, len);
    lua_call(ls, 2, 0);
}

static int std_print(lua_State* ls) {
    int top = lua_gettop(ls);
    lua_getglobal(ls, "stdout");
    for (int i = 1; i <= top; ++i) {
        if (i > 1) std_write(ls, top + 1, "\t", 1);
        size_t len;
        char const* s = luaL_tolstring(ls, i, &len);
        std_write(ls, top + 1, s, len);
        lua_pop(ls, 1);
    }
    std_write(ls, top + 1, "\n", 1);
    return 0;
}

static mlua_reg const std_stream_regs[] = {
    MLUA_REG(string, __name, "StdStream"),
#define X(n) MLUA_REG(function, n, std_stream_ ## n)
    X(read),
    X(write),
    X(flush),
    X(setvbuf),
#undef X
    {NULL},
};

static void create_stream(lua_State* ls, char const* name, FILE* stream) {
    FILE** v = lua_newuserdatauv(ls, sizeof(FILE*), 0);
    *v = stream;
    luaL_getmetatable(ls, "StdStream");
    lua_setmetatable(ls, -2);
    lua_setglobal(ls, name);
}

static void init_stdio(lua_State* ls) {
    // Create the StdStream class.
    luaL_newmetatable(ls, "StdStream");
    mlua_set_fields(ls, std_stream_regs, 0);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, -2, "__index");

    // Create global objects for stdin, stdout and stderr.
    create_stream(ls, "stdin", stdin);
    create_stream(ls, "stdout", stdout);
    create_stream(ls, "stderr", stderr);

    // Override the print function.
    lua_pushcfunction(ls, std_print);
    lua_setglobal(ls, "print");
}

#endif

static int pmain(lua_State* ls) {
    // Set up module loading.
    mlua_open_libs(ls);

    // Set up stdio streams.
#ifdef LIB_PICO_STDIO
    init_stdio(ls);
#endif

    // Require the main module.
    lua_getglobal(ls, "require");
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_MODULE));
    lua_call(ls, 1, 1);

    // If the main module has a main function, call it.
    if (try_getfield(ls, -1, MLUA_ESTR(MLUA_MAIN_FUNCTION)) == LUA_OK) {
        lua_remove(ls, -2);  // Remove main module
        lua_call(ls, 0, 0);
    } else {
        lua_pop(ls, 2);  // Remove error and main module
    }
    return 0;
}

void mlua_main() {
    // TODO: Use lua_newstate and set panic and warn handlers.
    lua_State* ls = luaL_newstate();
    lua_pushcfunction(ls, pmain);
    int err = lua_pcall(ls, 0, 0, 0);
    if (err != LUA_OK) {
        printf("ERROR: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
    }
    lua_close(ls);
}

__attribute__((weak)) int main() {
#ifdef LIB_PICO_STDIO
    stdio_init_all();
#endif
    mlua_main();
    return 0;
}
