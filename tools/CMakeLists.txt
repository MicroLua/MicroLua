# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

include(ExternalProject)

ExternalProject_Add(mlua_proj_gen
    PREFIX gen
    SOURCE_DIR "${MLUA_PATH}/tools/gen"
    BINARY_DIR "${CMAKE_BINARY_DIR}/tools/gen"
    CMAKE_ARGS
        "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
        "-DMLUA_PATH:PATH=${MLUA_PATH}"
        "-DMLUA_INT:STRING=${MLUA_INT}"
        "-DMLUA_FLOAT:STRING=${MLUA_FLOAT}"
    BUILD_ALWAYS 1  # Force dependency checking
    INSTALL_COMMAND ""
)
set(exe "${CMAKE_BINARY_DIR}/tools/gen/gen")
if(CMAKE_HOST_WIN32)
    string(APPEND exe ".exe")
endif()
add_executable(mlua_tool_gen IMPORTED GLOBAL)
set_property(TARGET mlua_tool_gen PROPERTY IMPORTED_LOCATION "${exe}")
add_dependencies(mlua_tool_gen mlua_proj_gen)
