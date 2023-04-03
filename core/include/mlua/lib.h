#ifndef _MLUA_CORE_LIB_H
#define _MLUA_CORE_LIB_H

#include <stddef.h>

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// A registry entry.
typedef struct mlua_lib {
    struct mlua_lib* next;
    char const* name;
    int (*open)(lua_State*);
} mlua_lib;

// Register a library.
void mlua_register_lib(mlua_lib* lib);

// Populate package.preload with all registered libraries.
void mlua_open_libs(lua_State* ls);

#ifdef __cplusplus
}
#endif

#endif
