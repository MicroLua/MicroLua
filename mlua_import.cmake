# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

# This is a copy of ${MLUA_PATH}/mlua_import.cmake. It can be dropped into an
# external project to locate MicroLua. It should be include()ed prior to
# project().

# Get MLUA_PATH from cache, environment or default.
set(MLUA_PATH_SRC "from cache")
if(NOT MLUA_PATH AND DEFINED ENV{MLUA_PATH})
    set(MLUA_PATH "$ENV{MLUA_PATH}")
    set(MLUA_PATH_SRC "from environment")
endif()
if(NOT MLUA_PATH)
    message(FATAL_ERROR "MicroLua not found; please set MLUA_PATH")
endif()
set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to MicroLua")

# Check that MLUA_PATH is sane.
get_filename_component(MLUA_PATH "${MLUA_PATH}" REALPATH
    BASE_DIR "${CMAKE_BINARY_DIR}")
if(NOT EXISTS "${MLUA_PATH}")
    message(FATAL_ERROR "Directory '${MLUA_PATH}' not found")
endif()
set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to MicroLua" FORCE)
set(MLUA_INIT_CMAKE "${MLUA_PATH}/mlua_init.cmake")
if(NOT EXISTS "${MLUA_INIT_CMAKE}")
    message(FATAL_ERROR "MicroLua not found in ${MLUA_PATH}; check MLUA_PATH")
endif()

# Import the build system.
include("${MLUA_INIT_CMAKE}")
