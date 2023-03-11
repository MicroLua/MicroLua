#ifndef _MLUA_LIB_H
#define _MLUA_LIB_H

#include <stddef.h>

#include "lua.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlua_lib {
    struct mlua_lib* next;
    char const* name;
    int (*open)(lua_State*);
} mlua_lib;

void mlua_register_lib(mlua_lib* lib);
void mlua_open_libs(lua_State* ls);

#ifdef __cplusplus
}
#endif

#endif
