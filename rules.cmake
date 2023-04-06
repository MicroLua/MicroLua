set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to the MicroLua sources SDK")
message("MLUA_PATH is ${MLUA_PATH}")
set(MLUA_LUA_SOURCE_DIR ${MLUA_PATH}/ext/lua)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${MLUA_PATH}/tools)

function(mlua_want_lua)
    if(NOT Lua_FOUND)
        find_package(Lua REQUIRED)
    endif()
endfunction()

function(mlua_core_filenames VAR GLOB)
    file(GLOB paths ${MLUA_LUA_SOURCE_DIR}/${GLOB})
    foreach(path IN LISTS paths)
        cmake_path(GET path FILENAME name)
        list(APPEND names ${name})
    endforeach()
    set(${VAR} ${names} PARENT_SCOPE)
endfunction()

function(mlua_core_copy FILENAMES DEST)
    foreach(name IN LISTS FILENAMES)
        configure_file(${MLUA_LUA_SOURCE_DIR}/${name} ${DEST}/${name} COPYONLY)
    endforeach()
endfunction()

function(mlua_core_luaconf DEST)
    file(READ ${MLUA_LUA_SOURCE_DIR}/luaconf.h LUACONF)
    string(REGEX REPLACE
        "\n([ \t]*#[ \t]*define[ \t]+LUA_(INT|FLOAT)_DEFAULT[ \t])"
        "\n// \\1" LUACONF "${LUACONF}")
    configure_file(${MLUA_PATH}/core/luaconf.in.h ${DEST}/luaconf.h)
endfunction()

function(mlua_add_core_library TARGET)
    pico_add_library(${TARGET})
    target_include_directories(${TARGET}_headers INTERFACE
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
    foreach(name IN LISTS ARGN)
        target_sources(${TARGET} INTERFACE
            ${CMAKE_CURRENT_BINARY_DIR}/src/${name}
        )
    endforeach()
endfunction()

function(mlua_register_module TARGET MOD SRC)
    if(NOT ${SRC} STREQUAL "NOSRC")
        mlua_want_lua()
        get_filename_component(SRC ${SRC} ABSOLUTE)
        set(DATA ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${MOD}.data.h)
        add_custom_command(
            OUTPUT ${DATA}
            DEPENDS ${SRC}
            COMMAND Lua ${MLUA_PATH}/tools/embed_lua.lua ${SRC} ${DATA}
            VERBATIM
        )
    endif()
    string(REPLACE "." "_" SYM ${MOD})
    set(REG_C ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${MOD}.c)
    configure_file(${MLUA_PATH}/core/lib_register.in.c ${REG_C})
    target_sources(${TARGET} INTERFACE ${REG_C} PRIVATE ${DATA})
    target_link_libraries(${TARGET} INTERFACE mlua_core mlua_core_main)
endfunction()

function(mlua_add_core_c_module_noreg MOD)
    mlua_add_core_library(mlua_mod_${MOD} ${ARGN})
    target_link_libraries(mlua_mod_${MOD} INTERFACE mlua_core)
endfunction()

function(mlua_add_core_c_module MOD)
    mlua_add_core_c_module_noreg(${MOD} ${ARGN})
    mlua_register_module(mlua_mod_${MOD} ${MOD} NOSRC)
endfunction()

function(mlua_add_c_module TARGET MOD)
    pico_add_library(${TARGET})
    target_sources(${TARGET} INTERFACE ${ARGN})
    mlua_register_module(${TARGET} ${MOD} NOSRC)
endfunction()

function(mlua_add_lua_module TARGET MOD SRC)
    pico_add_library(${TARGET})
    mlua_register_module(${TARGET} ${MOD} ${SRC})
endfunction()

set_property(GLOBAL PROPERTY mlua_test_targets)

function(mlua_add_lua_test_module TARGET MOD SRC)
    mlua_add_lua_module(${TARGET} ${MOD} ${SRC})
    get_property(tests GLOBAL PROPERTY mlua_test_targets)
    list(APPEND tests ${TARGET})
    set_property(GLOBAL PROPERTY mlua_test_targets ${tests})
endfunction()
