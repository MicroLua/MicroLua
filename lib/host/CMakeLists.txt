# Copyright 2024 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

target_include_directories(mlua_platform_luaconf INTERFACE include_luaconf)
target_include_directories(mlua_platform INTERFACE include_platform)
target_sources(mlua_platform INTERFACE platform.c)
