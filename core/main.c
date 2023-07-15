#include "mlua/main.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/timer.h"
#include "pico/platform.h"

#include "mlua/module.h"
#include "mlua/util.h"

// Silence link-time warnings.
__attribute__((weak)) int _link(char const* old, char const* new) { return -1; }
__attribute__((weak)) int _unlink(char const* file) { return -1; }

#ifdef LIB_PICO_STDIO

#include <unistd.h>
#include "pico/stdio.h"

static char const StdInStream_name[] = "mlua.StdInStream";
static char const StdOutStream_name[] = "mlua.StdOutStream";

static int StdInStream_read(lua_State* ls) {
    int fd = *((int*)luaL_checkudata(ls, 1, StdInStream_name));
    lua_Integer len = luaL_optinteger(ls, 2, -1);
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

static int StdOutStream_write(lua_State* ls) {
    int fd = *((int*)luaL_checkudata(ls, 1, StdOutStream_name));
    size_t len;
    char const* s = luaL_checklstring(ls, 2, &len);
    int cnt = write(fd, s, len);
    if (cnt < 0) return luaL_fileresult(ls, 0, NULL);
    lua_pushinteger(ls, cnt);
    return 1;
}

static void std_write(lua_State* ls, char const* s) {
    lua_getglobal(ls, "stdout");
    lua_getfield(ls, -1, "write");
    lua_rotate(ls, -2, 1);
    if (s != NULL) { lua_pushstring(ls, s); } else { lua_rotate(ls, -3, -1); }
    lua_call(ls, 2, 0);
}

static int std_print(lua_State* ls) {
    int top = lua_gettop(ls);
    for (int i = 1; i <= top; ++i) {
        if (i > 1) std_write(ls, "\t");
        luaL_tolstring(ls, i, NULL);
        std_write(ls, NULL);
    }
    std_write(ls, "\n");
    return 0;
}

static MLuaSym const StdInStream_syms[] = {
    MLUA_SYM_F(read, StdInStream_),
};

static MLuaSym const StdOutStream_syms[] = {
    MLUA_SYM_F(write, StdOutStream_),
};

static void create_stream(lua_State* ls, char const* name, char const* cls,
                          int stream) {
    int* v = lua_newuserdatauv(ls, sizeof(int), 0);
    *v = stream;
    luaL_getmetatable(ls, cls);
    lua_setmetatable(ls, -2);
    lua_setglobal(ls, name);
}

static void init_stdio(lua_State* ls) {
    // Create the StdInStream and StdOutStream classes.
    mlua_new_class(ls, StdInStream_name, StdInStream_syms);
    mlua_new_class(ls, StdOutStream_name, StdOutStream_syms);

    // Create global objects for stdin, stdout and stderr.
    create_stream(ls, "stdin", StdInStream_name, STDIN_FILENO);
    create_stream(ls, "stdout", StdOutStream_name, STDOUT_FILENO);
    create_stream(ls, "stderr", StdOutStream_name, STDERR_FILENO);

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

static int pmain(lua_State* ls) {
    // Register compiled-in modules.
    mlua_register_modules(ls);
    mlua_util_init(ls);

    // Set up stdio streams.
#ifdef LIB_PICO_STDIO
    init_stdio(ls);
#endif

    // Import the main module and get its main function.
    lua_pushcfunction(ls, getfield);
    lua_getglobal(ls, "require");
    lua_rotate(ls, 1, -1);
    lua_call(ls, 1, 1);
    lua_rotate(ls, 1, -1);
    bool has_main = lua_pcall(ls, 2, 1, 0) == LUA_OK;
    if (!has_main) lua_pop(ls, 1);  // Remove error

#if LIB_MLUA_MOD_MLUA_THREAD
    // The mlua.thread module is available. Start a thread for the main module's
    // main function, then call mlua.thread.main.
    mlua_require(ls, "mlua.thread", true);
    if (has_main) {
        lua_getfield(ls, -1, "start");  // Start the thread
        lua_rotate(ls, -3, -1);
        lua_call(ls, 1, 1);
        luaL_getmetafield(ls, -1, "set_name");  // Set the thread name
        lua_rotate(ls, -2, 1);
        lua_pushliteral(ls, "main");
        lua_call(ls, 2, 0);
    }
    lua_getfield(ls, -1, "main");  // Run thread.main
    lua_remove(ls, -2);  // Remove thread module
    lua_call(ls, 0, 0);
#else
    // The mlua.thread module isn't available. Call the main module's main
    // function directly.
    if (has_main) lua_call(ls, 0, 0);
#endif
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
    // Ensure that the system timer is ticking. This seems to take some time
    // after a reset.
    busy_wait_us(1);

    // Run the Lua interpreter.
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
