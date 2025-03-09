# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

message("MLUA_PATH is ${MLUA_PATH} (${MLUA_PATH_SRC})")
set(MLUA_LITTLEFS_SOURCE_DIR "${MLUA_PATH}/ext/littlefs" CACHE INTERNAL "")
set(MLUA_LUA_SOURCE_DIR "${MLUA_PATH}/ext/lua" CACHE INTERNAL "")
set(MLUA_LWIP_SOURCE_DIR "${MLUA_PATH}/ext/lwip" CACHE INTERNAL "")

function(mlua_set NAME DEFAULT)
    set(src "from cache")
    if("${${NAME}}" STREQUAL "")
        if(DEFINED ENV{${NAME}})
            set("${NAME}" "$ENV{${NAME}}")
            set(src "from environment")
        else()
            set("${NAME}" "${DEFAULT}")
            set(src "default")
        endif()
    endif()
    set("${NAME}" "${${NAME}}" ${ARGN} FORCE)
    message("${NAME} is \"${${NAME}}\" (${src})")
endfunction()

# Platform configuration.
mlua_set(MLUA_PLATFORM "pico" CACHE STRING "The target platform")
set_property(CACHE MLUA_PLATFORM PROPERTY STRINGS host pico)
set(platform_cmake "${MLUA_PATH}/lib/${MLUA_PLATFORM}/platform.cmake")
if(NOT EXISTS "${platform_cmake}")
    message(FATAL_ERROR "Invalid platform: ${MLUA_PLATFORM}")
endif()
include("${platform_cmake}")

# Lua interpreter configuration.
mlua_set(MLUA_INT "INT" CACHE STRING
    "The type of Lua integers, one of (INT, LONG, LONGLONG)")
set_property(CACHE MLUA_INT PROPERTY STRINGS INT LONG LONGLONG)
mlua_set(MLUA_FLOAT "FLOAT" CACHE STRING
    "The type of Lua numbers, one of (FLOAT, DOUBLE, LONGDOUBLE)")
set_property(CACHE MLUA_FLOAT PROPERTY STRINGS FLOAT DOUBLE LONGDOUBLE)

# Fennel compiler configuration.
mlua_set(MLUA_FENNEL "fennel" CACHE PATH "Path to the fennel compiler")

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

function(mlua_add_compile_options)
    add_compile_options(
        -Wall -Werror -Wextra -Wsign-compare -Wdouble-promotion
        -Wno-unused-function -Wno-unused-parameter -Wno-type-limits
    )
    # Lua >=5.5 triggers maybe-uninitialized in lstrlib.c.
    # https://groups.google.com/g/lua-l/c/BzpLl-8BvPk/m/cObYjVz7BwAJ
    add_compile_options(-Wno-maybe-uninitialized)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        # Lua triggers maybe-uninitialized in debug builds.
        add_compile_options(-Wno-maybe-uninitialized)
    endif()
endfunction()

function(mlua_list_targets VAR)
    cmake_parse_arguments(PARSE_ARGV 1 args "" "" "DIRS;EXCLUDE;INCLUDE")
    set(targets)
    foreach(dir IN LISTS args_DIRS)
        get_property(dir_targets DIRECTORY "${dir}"
                     PROPERTY BUILDSYSTEM_TARGETS)
        set(filtered)
        foreach(re IN LISTS args_INCLUDE)
            set(tgs ${dir_targets})
            list(FILTER tgs INCLUDE REGEX "${re}")
            list(APPEND filtered ${tgs})
        endforeach()
        list(REMOVE_DUPLICATES filtered)
        foreach(re IN LISTS args_EXCLUDE)
            list(FILTER filtered EXCLUDE REGEX "${re}")
        endforeach()
        list(APPEND targets ${filtered})
        get_property(subdirs DIRECTORY "${dir}" PROPERTY SUBDIRECTORIES)
        foreach(subdir IN LISTS subdirs)
            mlua_list_targets(sub_targets
                DIRS "${subdir}"
                INCLUDE ${args_INCLUDE} EXCLUDE ${args_EXCLUDE})
            list(APPEND targets ${sub_targets})
        endforeach()
    endforeach()
    set("${VAR}" "${targets}" PARENT_SCOPE)
endfunction()

function(mlua_fennel_version VERSION)
    execute_process(
        COMMAND "${MLUA_FENNEL}" "--version"
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

function(mlua_add_library TARGET)
    add_library("${TARGET}_headers" INTERFACE)
    add_library("${TARGET}" INTERFACE)
    string(TOUPPER "${TARGET}" utarget)
    string(REGEX REPLACE "[.+-]" "_" utarget "${utarget}")
    target_compile_definitions("${TARGET}" INTERFACE "LIB_${utarget}=1")
    target_link_libraries("${TARGET}" INTERFACE "${TARGET}_headers")
endfunction()

function(mlua_mirrored_target_link_libraries TARGET SCOPE)
    foreach(dep IN LISTS ARGN)
        target_link_libraries("${TARGET}_headers" "${SCOPE}" "${dep}_headers")
        target_link_libraries("${TARGET}" "${SCOPE}" "${dep}")
    endforeach()
endfunction()

function(mlua_add_core_library TARGET)
    mlua_add_library("${TARGET}")
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
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${MOD}_reg.c")
    configure_file("${MLUA_PATH}/core/module_core.in.c" "${output}")
    target_sources("mlua_mod_${MOD}" INTERFACE "${output}")
endfunction()

function(mlua_add_c_module TARGET)
    mlua_add_library("${TARGET}")
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
        mlua_add_gen_target("${TARGET}" mlua_gen_c INTERFACE "${output}")
    endforeach()
    target_link_libraries("${TARGET}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_header_module TARGET MOD SRC)
    cmake_parse_arguments(PARSE_ARGV 3 args "" "" "EXCLUDE;STRIP;TYPES")
    mlua_add_library("${TARGET}")
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
            TYPES "${args_TYPES}"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
    mlua_add_gen_target("${TARGET}" mlua_gen_header INTERFACE "${output}")
    target_link_libraries("${TARGET}" INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_lua_modules TARGET)
    mlua_add_library("${TARGET}")
    set(compile 1)
    foreach(SRC IN LISTS ARGN)
        if(SRC STREQUAL "NOCOMPILE")
            set(compile 0)
            continue()
        endif()
        cmake_path(ABSOLUTE_PATH SRC)
        cmake_path(GET SRC STEM LAST_ONLY MOD)
        set(template "${MLUA_PATH}/core/module_lua.in.c")
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${MOD}.c")
        if("${compile}")
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
    set(srcs)
    foreach(src IN LISTS ARGN)
        cmake_path(ABSOLUTE_PATH src)
        cmake_path(GET src STEM LAST_ONLY mod)
        set(output "${CMAKE_CURRENT_BINARY_DIR}/${mod}.lua")
        add_custom_command(
            COMMENT "Generating $<PATH:RELATIVE_PATH,${output},${CMAKE_BINARY_DIR}>"
            DEPENDS "${src}"
            OUTPUT "${output}"
            COMMAND "${MLUA_FENNEL}"
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

function(mlua_add_executable_no_config TARGET)
    add_executable("${TARGET}" ${ARGN})
    mlua_add_executable_platform("${TARGET}")
endfunction()

function(mlua_add_executable TARGET)
    mlua_add_executable_no_config("${TARGET}" ${ARGN})
    mlua_add_config_module("${TARGET}")
endfunction()
