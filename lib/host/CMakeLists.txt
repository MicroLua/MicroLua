# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

# Library: platform abstraction layer
target_include_directories(mlua_platform_luaconf INTERFACE include_luaconf)
target_include_directories(mlua_platform_headers INTERFACE include_platform)
target_sources(mlua_platform INTERFACE platform.c)

# Executable: standalone binary
function(mlua_platform_bin_standalone TARGET)
endfunction()

# Executable: unit tests
function(mlua_platform_bin_tests TARGET SUFFIX)
endfunction()

target_include_directories(mlua_mod_mlua.thread_headers INTERFACE
    include_mlua.thread)
target_sources(mlua_mod_mlua.thread INTERFACE event.c)
