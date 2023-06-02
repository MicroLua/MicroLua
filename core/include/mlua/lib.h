#ifndef _MLUA_CORE_LIB_H
#define _MLUA_CORE_LIB_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// A registry entry.
typedef struct mlua_lib {
    char const* name;
    lua_CFunction open;
} mlua_lib;

// Populate package.preload with all registered libraries.
void mlua_open_libs(lua_State* ls);

#ifdef __cplusplus
}
#endif

#endif
