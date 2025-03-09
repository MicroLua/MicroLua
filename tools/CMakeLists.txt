# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

include(ExternalProject)

function(mlua_add_tool TARGET BIN)
    if(CMAKE_HOST_WIN32)
        string(APPEND BIN ".exe")
    endif()
    add_executable("${TARGET}" IMPORTED GLOBAL)
    set_property(TARGET "${TARGET}" PROPERTY IMPORTED_LOCATION "${BIN}")
endfunction()

function(mlua_add_tool_project NAME)
    set(args)
    if(DEFINED MLUA_GEN)
        list(APPEND args "-DMLUA_GEN:PATH=${MLUA_GEN}")
    endif()
    set(proj "mlua_tools_${NAME}")
    ExternalProject_Add("${proj}"
        SOURCE_DIR "${MLUA_PATH}/tools/${NAME}"
        BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${NAME}"
        CMAKE_ARGS
            "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
            "-DMLUA_PATH:PATH=${MLUA_PATH}"
            "-DMLUA_PLATFORM:STRING=host"
            "-DMLUA_INT:STRING=${MLUA_INT}"
            "-DMLUA_FLOAT:STRING=${MLUA_FLOAT}"
            "-DMLUA_FENNEL:STRING=${MLUA_FENNEL}"
            ${args}
        BUILD_ALWAYS 1  # Force dependency checking
        INSTALL_COMMAND ""
    )
    if(DEFINED MLUA_GEN)
        add_dependencies("${proj}" mlua_tool_gen)
    endif()
    set(tool "mlua_tool_${NAME}")
    mlua_add_tool("${tool}" "${CMAKE_CURRENT_BINARY_DIR}/${NAME}/${NAME}")
    add_dependencies("${tool}" "${proj}")
endfunction()

if(NOT MLUA_BUILDING_TOOL STREQUAL "gen")
    if(DEFINED MLUA_GEN)
        mlua_add_tool(mlua_tool_gen "${MLUA_GEN}")
    else()
        mlua_add_tool_project(gen)
        set(MLUA_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen/gen" CACHE INTERNAL "")
    endif()
    if(NOT MLUA_BUILDING_TOOL STREQUAL "mlua")
        mlua_add_tool_project(mlua)
    endif()
endif()
