#ifndef _MLUA_CORE_MODULE_H
#define _MLUA_CORE_MODULE_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// A module registry entry.
typedef struct MLuaModule {
    char const* name;
    lua_CFunction open;
} MLuaModule;

// Populate package.preload with all comiled-in modules.
void mlua_register_modules(lua_State* ls);

#ifdef __cplusplus
}
#endif

#endif
