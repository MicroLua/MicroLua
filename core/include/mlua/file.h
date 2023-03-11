#ifndef _MLUA_FILE_H
#define _MLUA_FILE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mlua_file {
    struct mlua_file* next;
    char const* path;
    char const* start;
    char const* end;
} mlua_file;

void mlua_add_file(mlua_file* f);
char const* mlua_get_file(char const* path, size_t* size);

#ifdef __cplusplus
}
#endif

#endif
