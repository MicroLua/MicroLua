# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

message("MLUA_PATH is ${MLUA_PATH}")
set(MLUA_LITTLEFS_SOURCE_DIR "${MLUA_PATH}/ext/littlefs" CACHE INTERNAL "")
set(MLUA_LUA_SOURCE_DIR "${MLUA_PATH}/ext/lua" CACHE INTERNAL "")

function(mlua_set NAME DEFAULT)
    if("${${NAME}}" STREQUAL "")
        set("${NAME}" "${DEFAULT}")
    endif()
    set("${NAME}" "${${NAME}}" ${ARGN})
endfunction()

function(mlua_add_compile_options)
    add_compile_options(
        -Wall -Werror -Wextra -Wsign-compare -Wdouble-promotion
        -Wno-unused-function -Wno-unused-parameter
    )
endfunction()

# Configure the interpreter.
mlua_set(MLUA_INT INT CACHE STRING
    "The type of Lua integers, one of (INT, LONG, LONGLONG)")
set_property(CACHE MLUA_INT PROPERTY STRINGS INT LONG LONGLONG)
mlua_set(MLUA_FLOAT FLOAT CACHE STRING
    "The type of Lua numbers, one of (FLOAT, DOUBLE, LONGDOUBLE)")
set_property(CACHE MLUA_FLOAT PROPERTY STRINGS FLOAT DOUBLE LONGDOUBLE)
mlua_set(MLUA_COMPILE 1 CACHE BOOL "Compile Lua sources at build-time")

# Copy Lua sources. This is necessary to allow overriding luaconf.h.
file(GLOB paths "${MLUA_LUA_SOURCE_DIR}/*.[hc]")
foreach(path IN LISTS paths)
    cmake_path(GET path FILENAME name)
    if(NOT name STREQUAL "luaconf.h")
        configure_file("${path}" "${CMAKE_BINARY_DIR}/ext/lua/${name}" COPYONLY)
    endif()
endforeach()

# Generate luaconf.h.
file(READ "${MLUA_LUA_SOURCE_DIR}/luaconf.h" LUACONF)
string(REGEX REPLACE
    "\n([ \t]*#[ \t]*define[ \t]+LUA_(INT|FLOAT)_DEFAULT[ \t])"
    "\n// \\1" LUACONF "${LUACONF}")
configure_file("${MLUA_PATH}/core/luaconf.in.h"
    "${CMAKE_BINARY_DIR}/ext/lua/luaconf.h")

if(NOT FENNEL)
    if(DEFINED ENV{FENNEL})
        set(FENNEL "$ENV{FENNEL}")
        message("Using FENNEL from environment ('${FENNEL}')")
    else()
        set(FENNEL "fennel")
    endif()
endif()
set(FENNEL "${FENNEL}" CACHE PATH "Path to the fennel compiler" FORCE)
message("FENNEL is ${FENNEL}")

function(mlua_fennel_version VERSION)
    execute_process(
        COMMAND "${FENNEL}" "--version"
        OUTPUT_VARIABLE version
        RESULT_VARIABLE result
    )
    if("${result}" EQUAL 0)
        string(STRIP "${version}" version)
        set("${VERSION}" "${version}" PARENT_SCOPE)
    endif()
endfunction()

function(mlua_want_fennel)
    if(NOT TARGET mlua_Fennel_FOUND)
        mlua_fennel_version(version)
        if(version STREQUAL "")
            message(FATAL_ERROR "Fennel compiler not found")
        endif()
        message("Fennel compiler is: ${version}")
        add_library(mlua_Fennel_FOUND INTERFACE)
    endif()
endfunction()

function(mlua_num_target VAR PREFIX)
    set(prop "${PREFIX}_id")
    get_property(set GLOBAL PROPERTY "${prop}" SET)
    if(NOT set)
        set_property(GLOBAL PROPERTY "${prop}" 0)
    endif()
    get_property(id GLOBAL PROPERTY "${prop}")
    set("${VAR}" "${PREFIX}_${id}" PARENT_SCOPE)
    math(EXPR id "${id} + 1")
    set_property(GLOBAL PROPERTY "${prop}" "${id}")
endfunction()

function(mlua_add_gen_target TARGET PREFIX SCOPE)
    mlua_num_target(gtarget "${PREFIX}")
    add_custom_target("${gtarget}" DEPENDS "${ARGN}")
    add_dependencies("${TARGET}" "${gtarget}")
    target_sources("${TARGET}" "${SCOPE}" "${ARGN}")
endfunction()

function(mlua_add_core_library TARGET)
    pico_add_library("${TARGET}")
    target_include_directories("${TARGET}_headers" INTERFACE
        "${CMAKE_BINARY_DIR}/ext/lua"
    )
    foreach(name IN LISTS ARGN)
        target_sources("${TARGET}" INTERFACE
            "${CMAKE_BINARY_DIR}/ext/lua/${name}"
        )
    endforeach()
endfunction()

function(mlua_add_core_c_module_noreg MOD)
    mlua_add_core_library("mlua_mod_${MOD}" "${ARGN}")
    target_link_libraries("mlua_mod_${MOD}" INTERFACE mlua_core)
endfunction()

function(mlua_add_core_c_module MOD)
    mlua_add_core_c_module_noreg("${MOD}" "${ARGN}")
    set(template "${MLUA_PATH}/core/module_core.in.c")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${MOD}_reg.c")
    add_custom_command(
        COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
        DEPENDS mlua_tool_gen "${template}"
        OUTPUT "${output}"
        COMMAND mlua_tool_gen
            "coremod" "${MOD}" "${template}" "${output}"
        VERBATIM
    )
    mlua_add_gen_target("mlua_mod_${MOD}" mlua_gen_core INTERFACE "${output}")
    target_link_libraries("mlua_mod_${MOD}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_c_module TARGET)
    pico_add_library("${TARGET}")
    foreach(src IN LISTS ARGN)
        cmake_path(ABSOLUTE_PATH src)
        cmake_path(GET src FILENAME name)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${name}")
        add_custom_command(
            COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
            DEPENDS mlua_tool_gen "${src}"
            OUTPUT "${output}"
            COMMAND mlua_tool_gen
                "cmod" "${src}" "${output}"
            VERBATIM
        )
        mlua_add_gen_target("${TARGET}" mlua_gen_core INTERFACE "${output}")
    endforeach()
    target_link_libraries("${TARGET}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_header_module TARGET MOD SRC)
    cmake_parse_arguments(PARSE_ARGV 3 args "" "" "EXCLUDE;STRIP")
    pico_add_library("${TARGET}")
    cmake_path(ABSOLUTE_PATH SRC)
    set(template "${MLUA_PATH}/core/module_header.in.c")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
    set(incdirs "$<TARGET_PROPERTY:${TARGET},INTERFACE_INCLUDE_DIRECTORIES>")
    add_custom_command(
        COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
        DEPENDS mlua_tool_gen "${SRC}" "${template}"
        OUTPUT "${output}"
        COMMAND "${CMAKE_C_COMPILER}" -E -dD
            "$<$<BOOL:${incdirs}>:-I$<JOIN:${incdirs},;-I>>"
            -o "${output}.syms" "${SRC}"
        COMMAND mlua_tool_gen
            "headermod" "${MOD}" "${SRC}" "${output}.syms" "${template}"
            "${output}" EXCLUDE "${args_EXCLUDE}" STRIP "${args_STRIP}"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    mlua_add_gen_target("${TARGET}" mlua_gen_header INTERFACE "${output}")
    target_link_libraries("${TARGET}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_lua_modules TARGET)
    pico_add_library("${TARGET}")
    foreach(SRC IN LISTS ARGN)
        cmake_path(ABSOLUTE_PATH SRC)
        cmake_path(GET SRC STEM LAST_ONLY MOD)
        set(template "${MLUA_PATH}/core/module_lua.in.c")
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
        if("${MLUA_COMPILE}")
            add_custom_command(
                COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
                DEPENDS mlua_tool_gen "${SRC}" "${template}"
                OUTPUT "${output}"
                COMMAND mlua_tool_gen
                    "luamod" "${MOD}" "${SRC}" "${template}" "${output}"
                VERBATIM
            )
            mlua_add_gen_target("${TARGET}" mlua_gen_lua INTERFACE "${output}")
        else()
            set(INCBIN "1")
            configure_file("${template}" "${output}")
            set_source_files_properties("${output}" OBJECT_DEPENDS "${SRC}")
            target_sources("${TARGET}" INTERFACE "${output}")
        endif()
    endforeach()
    target_link_libraries("${TARGET}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_fnl_modules TARGET)
    mlua_want_fennel()
    foreach(src IN LISTS ARGN)
        cmake_path(ABSOLUTE_PATH src)
        cmake_path(GET src STEM LAST_ONLY mod)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${mod}.lua")
        add_custom_command(
            COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
            DEPENDS "${src}"
            OUTPUT "${output}"
            COMMAND "${FENNEL}"
                "--compile" "--correlate" "${src}" > "${output}"
            VERBATIM
        )
        list(APPEND srcs "${output}")
    endforeach()
    mlua_add_lua_modules("${TARGET}" "${srcs}")
endfunction()

# TARGET must be a binary target.
function(mlua_add_config_module TARGET)
    set(template "${MLUA_PATH}/core/module_config.in.c")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/mlua.config_${TARGET}.c")
    add_custom_command(
        COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
        DEPENDS mlua_tool_gen "${template}"
        OUTPUT "${output}"
        COMMAND mlua_tool_gen
            "configmod" "mlua.config" "${template}" "${output}"
            "$<TARGET_PROPERTY:${TARGET},mlua_config_symbols>"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    mlua_add_gen_target("${TARGET}" mlua_gen_config PRIVATE "${output}")
    target_link_libraries("${TARGET}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_target_config TARGET)
    set_property(TARGET "${TARGET}"
                 APPEND PROPERTY mlua_config_symbols "${ARGN}")
endfunction()

set_property(GLOBAL PROPERTY mlua_test_targets)

function(mlua_add_lua_test_modules TARGET)
    mlua_add_lua_modules("${TARGET}" ${ARGN})
    set_property(GLOBAL APPEND PROPERTY mlua_test_targets "${TARGET}")
endfunction()

include(ExternalProject)

function(mlua_add_tool TARGET SRC BIN)
    cmake_path(GET BIN PARENT_PATH bin_dir)
    ExternalProject_Add("${TARGET}_ext"
        SOURCE_DIR "${SRC}"
        BINARY_DIR "${bin_dir}"
        CMAKE_ARGS
            "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
            "-DMLUA_PATH:PATH=${MLUA_PATH}"
            "-DMLUA_INT:STRING=${MLUA_INT}"
            "-DMLUA_FLOAT:STRING=${MLUA_FLOAT}"
        BUILD_ALWAYS 1  # Force dependency checking
        INSTALL_COMMAND ""
    )
    if(CMAKE_HOST_WIN32)
        string(APPEND BIN ".exe")
    endif()
    add_executable("${TARGET}" IMPORTED GLOBAL)
    set_property(TARGET "${TARGET}" PROPERTY IMPORTED_LOCATION "${BIN}")
    add_dependencies("${TARGET}" "${TARGET}_ext")
endfunction()

function(mlua_add_lua_tool TARGET SCRIPT)
    cmake_path(GET SCRIPT FILENAME FILE)
    set(main "${CMAKE_CURRENT_BINARY_DIR}/${TARGET}_main.c")
    configure_file("${MLUA_PATH}/core/tool_main.in.c" "${main}")
    set_source_files_properties("${main}" OBJECT_DEPENDS "${SCRIPT}")
    add_executable("${TARGET}"
        "${CMAKE_BINARY_DIR}/ext/lua/onelua.c"
        "${main}"
    )
    target_compile_definitions("${TARGET}" PRIVATE
        MAKE_LIB
    )
    target_include_directories("${TARGET}" PRIVATE
        "${CMAKE_BINARY_DIR}/ext/lua"
        "${MLUA_PATH}/core/include_tool"
    )
    target_link_libraries("${TARGET}" PRIVATE m)
    if(CMAKE_HOST_WIN32)
        target_compile_definitions("${TARGET}" PRIVATE LUA_USE_WINDOWS)
    else()
        target_compile_definitions("${TARGET}" PRIVATE LUA_USE_LINUX)
    endif()
endfunction()
