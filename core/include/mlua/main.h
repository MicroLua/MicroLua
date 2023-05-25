#ifndef _MLUA_CORE_MAIN_H
#define _MLUA_CORE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

// Write a string with an optional "%s" placeholder for the parameter to stderr.
void mlua_writestringerror(char const* fmt, char const* param);

// The entry point for a Lua interpreter instance.
struct mlua_entry_point {
    char const* module;
    char const* func;
};

// Set up the Lua interpreter and start the main module.
void mlua_main(struct mlua_entry_point const* ep);

#ifdef __cplusplus
}
#endif

#endif
