#ifndef _MLUA_CORE_MAIN_H
#define _MLUA_CORE_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

// Write a string with an optional "%s" placeholder for the parameter to stderr.
void mlua_writestringerror(char const* fmt, char const* param);

// Set up the Lua interpreter and start the main module.
void mlua_main();

#ifdef __cplusplus
}
#endif

#endif
