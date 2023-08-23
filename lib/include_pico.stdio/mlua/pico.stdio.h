#ifndef _MLUA_LIB_PICO_STDIO_H
#define _MLUA_LIB_PICO_STDIO_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// Read from the stream fd. The number of bytes to read is at the given index.
int mlua_pico_stdio_read(lua_State* ls, int fd, int index);

// Write the string at the given index to the stream fd.
int mlua_pico_stdio_write(lua_State* ls, int fd, int index);

#ifdef __cplusplus
}
#endif

#endif
