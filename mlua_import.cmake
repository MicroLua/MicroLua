# This is a copy of ${MLUA_PATH}/mlua_import.cmake. It can be dropped into an
# external project to locate MicroLua. It should be include()ed prior to
# project().

if(DEFINED ENV{MLUA_PATH} AND (NOT MLUA_PATH))
    set(MLUA_PATH $ENV{MLUA_PATH})
    message("Using MLUA_PATH from environment ('${MLUA_PATH}')")
endif()

set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to MicroLua")
if(NOT MLUA_PATH)
    message(FATAL_ERROR
        "MicroLua location was not specified; please set MLUA_PATH")
endif()

get_filename_component(MLUA_PATH "${MLUA_PATH}" REALPATH
    BASE_DIR "${CMAKE_BINARY_DIR}")
if(NOT EXISTS "${MLUA_PATH}")
    message(FATAL_ERROR "Directory '${MLUA_PATH}' not found")
endif()

set(MLUA_INIT_CMAKE_FILE "${MLUA_PATH}/mlua_init.cmake")
if (NOT EXISTS "${MLUA_INIT_CMAKE_FILE}")
    message(FATAL_ERROR
        "Directory '${MLUA_PATH}' does not appear to contain MicroLua")
endif()

set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to MicroLua" FORCE)

include("${MLUA_INIT_CMAKE_FILE}")
