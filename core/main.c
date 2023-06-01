#include "mlua/main.h"

#include <stdlib.h>
#include <string.h>

#include "pico/platform.h"

#include "mlua/lib.h"
#include "mlua/util.h"

#ifdef LIB_PICO_STDIO

#include <unistd.h>
#include "pico/stdio.h"

static int StdStream_read(lua_State* ls) {
    int fd = *((int*)luaL_checkudata(ls, 1, "StdStream"));
    lua_Integer len = luaL_optinteger(ls, 2, -1);
    if (fd != STDIN_FILENO) return luaL_error(ls, "read from non-input stream");
    if (len <= 0) {
        lua_pushlstring(ls, "", 0);
        return 1;
    }
    luaL_Buffer buf;
    char* p = luaL_buffinitsize(ls, &buf, len);
    int cnt = read(fd, p, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    luaL_pushresultsize(&buf, cnt);
    return 1;
}

static int StdStream_write(lua_State* ls) {
    int fd = *((int*)luaL_checkudata(ls, 1, "StdStream"));
    if (fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        return luaL_error(ls, "write to non-output stream");
    }
    size_t len;
    char const* s = luaL_checklstring(ls, 2, &len);
    int cnt = write(fd, s, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    lua_pushinteger(ls, cnt);
    return 1;
}

static void std_write(lua_State* ls, int index, char const* s, size_t len) {
    index = lua_absindex(ls, index);
    lua_pushvalue(ls, index + 1);
    lua_pushvalue(ls, index);
    lua_pushlstring(ls, s, len);
    lua_call(ls, 2, 0);
}

static int std_print(lua_State* ls) {
    int top = lua_gettop(ls);
    lua_getglobal(ls, "stdout");
    lua_getfield(ls, -1, "write");
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

static mlua_reg const StdStream_regs[] = {
#define MLUA_SYM(n, v) MLUA_REG(string, n, v)
    MLUA_SYM(__name, "StdStream"),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, StdStream_ ## n)
    MLUA_SYM(read),
    MLUA_SYM(write),
#undef MLUA_SYM
    {NULL},
};

static void create_stream(lua_State* ls, char const* name, int stream) {
    int* v = lua_newuserdatauv(ls, sizeof(int), 0);
    *v = stream;
    luaL_getmetatable(ls, "StdStream");
    lua_setmetatable(ls, -2);
    lua_setglobal(ls, name);
}

static void init_stdio(lua_State* ls) {
    // Create the StdStream class.
    luaL_newmetatable(ls, "StdStream");
    mlua_set_fields(ls, StdStream_regs, 0);
    lua_pushvalue(ls, -1);
    lua_setfield(ls, -2, "__index");

    // Create global objects for stdin, stdout and stderr.
    create_stream(ls, "stdin", STDIN_FILENO);
    create_stream(ls, "stdout", STDOUT_FILENO);
    create_stream(ls, "stderr", STDERR_FILENO);

    // Override the print function.
    lua_pushcfunction(ls, std_print);
    lua_setglobal(ls, "print");
}

#endif  // LIB_PICO_STDIO

void mlua_writestringerror(char const* fmt, char const* param) {
    int i = 0;
    for (;;) {
        char c = fmt[i];
        if (c == '\0') {
            if (i > 0) fwrite(fmt, sizeof(char), i, stderr);
            fflush(stderr);
            return;
        } else if (c == '%' && fmt[i + 1] == 's') {
            if (i > 0) fwrite(fmt, sizeof(char), i, stderr);
            int plen = strlen(param);
            if (plen > 0) fwrite(param, sizeof(char), plen, stderr);
            fmt += i + 2;
            i = 0;
        } else {
            ++i;
        }
    }
}

static int getfield(lua_State* ls) {
    lua_gettable(ls, 1);
    return 1;
}

static int require_mlua_thread(lua_State* ls) {
    mlua_require(ls, "mlua.thread", true);
    return 1;
}

static int pmain(lua_State* ls) {
    // Set up module loading.
    mlua_open_libs(ls);

    // Set up stdio streams.
#ifdef LIB_PICO_STDIO
    init_stdio(ls);
#endif

    // Require the main module.
    lua_getglobal(ls, "require");
    lua_pushvalue(ls, 1);
    lua_call(ls, 1, 1);

    // Get the main module's main function.
    lua_pushcfunction(ls, getfield);
    lua_pushvalue(ls, -2);
    lua_pushvalue(ls, 2);
    if (lua_pcall(ls, 2, 1, 0) != LUA_OK) {
        lua_pop(ls, 1);  // Remove error
        lua_pushnil(ls);
    }
    lua_remove(ls, -2);  // Remove main module
    lua_remove(ls, 1);  // Remove module name
    lua_remove(ls, 1);  // Remove function name

    // If the "thread" module is available, run its main function, passing
    // the main module's main function as a task. Otherwise, run the main
    // module's main function on its own.
    lua_pushcfunction(ls, require_mlua_thread);
    if (lua_pcall(ls, 0, 1, 0) == LUA_OK) {
        lua_getfield(ls, -1, "main");
        lua_remove(ls, -2);  // Remove thread module
        lua_rotate(ls, -2, 1);
        lua_call(ls, 1, 0);
    } else if (!lua_isnil(ls, -2)) {
        lua_pop(ls, 1);  // Remove error
        lua_call(ls, 0, 0);
    }
    return 0;
}

static void* allocate(void* ud, void* ptr, size_t old_size, size_t new_size) {
    if (new_size != 0) return realloc(ptr, new_size);
    free(ptr);
    return NULL;
}

static int on_panic(lua_State* ls) {
    char const* msg = lua_tostring(ls, -1);
    if (msg == NULL) msg = "unknown error";
    mlua_writestringerror("PANIC: %s\n", msg);
    panic(NULL);
}

static void warn_print(char const* msg, bool first, bool last) {
    if (first) mlua_writestringerror("WARNING: ", "");
    mlua_writestringerror("%s", msg);
    if (last) mlua_writestringerror("\n", "");
}

static void on_warn_cont(void* ud, char const* msg, int cont);
static void on_warn_off(void* ud, char const* msg, int cont);

static void on_warn_on(void* ud, char const* msg, int cont) {
    if (!cont && msg[0] == '@') {
        if (strcmp(msg, "@off") == 0)
            lua_setwarnf((lua_State*)ud, &on_warn_off, ud);
        return;
    }
    warn_print(msg, true, !cont);
    if (cont) lua_setwarnf((lua_State*)ud, &on_warn_cont, ud);
}

static void on_warn_cont(void* ud, char const* msg, int cont) {
    warn_print(msg, false, !cont);
    if (!cont) lua_setwarnf((lua_State*)ud, &on_warn_on, ud);
}

static void on_warn_off(void* ud, char const* msg, int cont) {
    if (cont || strcmp(msg, "@on") == 0) return;
    lua_setwarnf((lua_State*)ud, &on_warn_on, ud);
}

lua_State* mlua_new_interpreter() {
    lua_State* ls = lua_newstate(allocate, NULL);
    lua_atpanic(ls, &on_panic);
    lua_setwarnf(ls, &on_warn_off, ls);
    return ls;
}

void mlua_run_main(lua_State* ls) {
    lua_pushcfunction(ls, pmain);
    lua_rotate(ls, 1, 1);
    int err = lua_pcall(ls, 2, 0, 0);
    if (err != LUA_OK) {
        mlua_writestringerror("ERROR: %s\n", lua_tostring(ls, -1));
        lua_pop(ls, 1);
    }
}

void mlua_main_core0() {
    lua_State* ls = mlua_new_interpreter();
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_MODULE));
    lua_pushliteral(ls, MLUA_ESTR(MLUA_MAIN_FUNCTION));
    mlua_run_main(ls);
    lua_close(ls);
}

__attribute__((weak)) int main() {
#ifdef LIB_PICO_STDIO
    stdio_init_all();
#endif
    mlua_main_core0();
    return 0;
}
