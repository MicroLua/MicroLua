#ifndef _MLUA_CORE_MODULE_H
#define _MLUA_CORE_MODULE_H

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

// A module registry entry.
struct mlua_module {
    char const* name;
    lua_CFunction open;
};

// Populate package.preload with all comiled-in modules.
void mlua_register_modules(lua_State* ls);

#ifdef __cplusplus
}
#endif

#endif
