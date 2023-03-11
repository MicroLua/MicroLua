#include "mlua/file.h"

#include <string.h>

// Silence link-time warnings.
__attribute__((weak)) int _link(char const* old, char const* new) { return -1; }
__attribute__((weak)) int _unlink(char const* file) { return -1; }

static mlua_file* files = NULL;

void mlua_add_file(mlua_file* f) {
    f->next = files;
    files = f;
}

char const* mlua_get_file(char const* path, size_t* size) {
    mlua_file* f = files;
    while (f != NULL) {
        if (strcmp(path, f->path) == 0) {
            *size = f->end - f->start;
            return f->start;
        }
        f = f->next;
    }
    return NULL;
}
