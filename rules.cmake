set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to the MicroLua sources SDK")
message("MLUA_PATH is ${MLUA_PATH}")
set(MLUA_LUA_SOURCE_DIR "${MLUA_PATH}/ext/lua")
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${MLUA_PATH}/tools")

function(mlua_set NAME DEFAULT)
    if("${${NAME}}" STREQUAL "")
        set("${NAME}" "${DEFAULT}")
    endif()
    set("${NAME}" "${${NAME}}" ${ARGN})
endfunction()

function(mlua_want_lua)
    if(NOT Lua_FOUND)
        find_package(Lua REQUIRED)
    endif()
endfunction()

function(mlua_core_filenames VAR GLOB)
    file(GLOB paths "${MLUA_LUA_SOURCE_DIR}/${GLOB}")
    foreach(path IN LISTS paths)
        cmake_path(GET path FILENAME name)
        list(APPEND names "${name}")
    endforeach()
    set("${VAR}" "${names}" PARENT_SCOPE)
endfunction()

function(mlua_core_copy FILENAMES DEST)
    foreach(name IN LISTS FILENAMES)
        configure_file("${MLUA_LUA_SOURCE_DIR}/${name}" "${DEST}/${name}"
                       COPYONLY)
    endforeach()
endfunction()

mlua_set(MLUA_INT INT CACHE STRING
    "The type of Lua integers, one of (INT, LONG, LONGLONG)")
set_property(CACHE MLUA_INT PROPERTY STRINGS INT LONG LONGLONG)
mlua_set(MLUA_FLOAT FLOAT CACHE STRING
    "The type of Lua numbers, one of (FLOAT, DOUBLE, LONGDOUBLE)")
set_property(CACHE MLUA_FLOAT PROPERTY STRINGS FLOAT DOUBLE LONGDOUBLE)

function(mlua_core_luaconf DEST)
    file(READ "${MLUA_LUA_SOURCE_DIR}/luaconf.h" LUACONF)
    string(REGEX REPLACE
        "\n([ \t]*#[ \t]*define[ \t]+LUA_(INT|FLOAT)_DEFAULT[ \t])"
        "\n// \\1" LUACONF "${LUACONF}")
    configure_file("${MLUA_PATH}/core/luaconf.in.h" "${DEST}/luaconf.h")
endfunction()

function(mlua_add_core_library TARGET)
    pico_add_library("${TARGET}")
    target_include_directories("${TARGET}_headers" INTERFACE
        "${CMAKE_CURRENT_BINARY_DIR}/include"
    )
    foreach(name IN LISTS ARGN)
        target_sources("${TARGET}" INTERFACE
            "${CMAKE_CURRENT_BINARY_DIR}/src/${name}"
        )
    endforeach()
endfunction()

function(mlua_register_module TARGET SCOPE MOD)
    cmake_parse_arguments(PARSE_ARGV 3 args "C" "CONFIG;HEADER;LUA" "")
    if(args_C)
        # Nothing to do
    elseif(DEFINED args_LUA)
        mlua_want_lua()
        get_filename_component(SRC "${args_LUA}" ABSOLUTE)
        set(DATA "${CMAKE_CURRENT_BINARY_DIR}/module_register_${MOD}.data.h")
        add_custom_command(
            COMMENT "Compiling Lua source ${DATA}"
            OUTPUT "${DATA}"
            DEPENDS "${SRC}"
            COMMAND Lua "${MLUA_PATH}/tools/embed_lua.lua" "${SRC}" "${DATA}"
            VERBATIM
        )
        target_sources("${TARGET}" "${SCOPE}" "${DATA}")
    elseif(DEFINED args_CONFIG)
        mlua_want_lua()
        set(SYMBOLS "${CMAKE_CURRENT_BINARY_DIR}/module_register_${MOD}.syms.h")
        add_custom_command(
            COMMENT "Generating symbols for config ${SYMBOLS}"
            OUTPUT "${SYMBOLS}"
            COMMAND Lua "${MLUA_PATH}/tools/embed_config.lua"
                "${SYMBOLS}" "${args_CONFIG}"
            COMMAND_EXPAND_LISTS
            VERBATIM
        )
        target_sources("${TARGET}" "${SCOPE}" "${SYMBOLS}")
    elseif(DEFINED args_HEADER)
        mlua_want_lua()
        get_filename_component(SRC "${args_HEADER}" ABSOLUTE)
        set(INCLUDE "#include \"${SRC}\"")
        set(SYMBOLS "${CMAKE_CURRENT_BINARY_DIR}/module_register_${MOD}.syms.h")
        add_custom_command(
            COMMENT "Generating symbols for header ${SYMBOLS}"
            OUTPUT "${SYMBOLS}"
            DEPENDS "${SRC}"
            COMMAND "${CMAKE_C_COMPILER}" -E -dD -o "${SYMBOLS}.names" "${SRC}"
            COMMAND Lua "${MLUA_PATH}/tools/embed_header.lua"
                "${SYMBOLS}.names" "${SYMBOLS}"
            VERBATIM
        )
        target_sources("${TARGET}" "${SCOPE}" "${SYMBOLS}")
    else()
        message(FATAL_ERROR "No module type found")
    endif()
    string(REPLACE "." "_" SYM "${MOD}")
    set(REG_C "${CMAKE_CURRENT_BINARY_DIR}/module_register_${MOD}.c")
    configure_file("${MLUA_PATH}/core/module_register.in.c" "${REG_C}")
    target_sources("${TARGET}" "${SCOPE}" "${REG_C}")
    target_link_libraries("${TARGET}" "${SCOPE}" mlua_core mlua_core_main)
endfunction()

function(mlua_add_core_c_module_noreg MOD)
    mlua_add_core_library("mlua_mod_${MOD}" "${ARGN}")
    target_link_libraries("mlua_mod_${MOD}" INTERFACE mlua_core)
endfunction()

function(mlua_add_core_c_module MOD)
    mlua_add_core_c_module_noreg("${MOD}" "${ARGN}")
    mlua_register_module("mlua_mod_${MOD}" INTERFACE "${MOD}" C)
endfunction()

function(mlua_add_c_module TARGET MOD)
    pico_add_library("${TARGET}")
    target_sources("${TARGET}" INTERFACE "${ARGN}")
    mlua_register_module("${TARGET}" INTERFACE "${MOD}" C)
endfunction()

function(mlua_add_header_module TARGET MOD SRC)
    pico_add_library("${TARGET}")
    mlua_register_module("${TARGET}" INTERFACE "${MOD}" HEADER "${SRC}")
endfunction()

function(mlua_add_lua_modules TARGET)
    pico_add_library("${TARGET}")
    foreach(SRC IN LISTS ARGN)
        get_filename_component(MOD "${SRC}" NAME_WLE)
        mlua_register_module("${TARGET}" INTERFACE "${MOD}" LUA "${SRC}")
    endforeach()
endfunction()

# TARGET must be a binary target.
function(mlua_add_config_module TARGET)
    mlua_register_module("${TARGET}" PRIVATE mlua.config CONFIG
                         "$<TARGET_PROPERTY:${TARGET},mlua_config_symbols>")
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
