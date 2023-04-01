set(MLUA_PATH "${MLUA_PATH}" CACHE PATH "Path to the MicroLua sources SDK")
message("MLUA_PATH is ${MLUA_PATH}")
set(MLUA_LUA_SOURCE_DIR ${MLUA_PATH}/ext/lua)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${MLUA_PATH}/tools)

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

function(mlua_add_core_headers_library TARGET)
    add_library(${TARGET}_headers INTERFACE)
    target_include_directories(${TARGET}_headers INTERFACE
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
endfunction()

function(mlua_add_core_library TARGET)
    mlua_add_core_headers_library(${TARGET})
    pico_add_impl_library(${TARGET})
endfunction()

function(mlua_core_target_sources TARGET)
    foreach(name IN LISTS ARGN)
        target_sources(${TARGET} INTERFACE
            ${CMAKE_CURRENT_BINARY_DIR}/src/${name}
        )
    endforeach()
endfunction()

function(mlua_add_core_lib_library LIB SRC)
    string(REPLACE "." "_" SYM ${LIB})
    set(TARGET mlua_core_lib_${SYM})
    mlua_add_core_library(${TARGET})
    mlua_core_target_sources(${TARGET} ${SRC})
    target_link_libraries(${TARGET} INTERFACE mlua_core)
    if(${ARGC} GREATER 2)
        if(NOT "${ARGV2}" STREQUAL "NOREGISTER")
            message(FATAL_ERROR "Unknown parameter ${ARGV2}")
        endif()
    else()
        set(REG_C ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${SYM}.c)
        configure_file(${MLUA_PATH}/core/lib_register.in.c ${REG_C})
        target_sources(${TARGET} INTERFACE ${REG_C})
    endif()
endfunction()

function(mlua_want_lua)
    if(NOT Lua_FOUND)
        find_package(Lua REQUIRED)
    endif()
endfunction()

function(mlua_add_lua_library TARGET LIB SRC)
    string(REPLACE "." "_" SYM ${LIB})
    # Convert the source to C array data.
    mlua_want_lua()
    set(DATA ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${SYM}.data.h)
    add_custom_command(
        OUTPUT ${DATA}
        DEPENDS ${SRC}
        COMMAND Lua ${MLUA_PATH}/tools/embed_lua.lua ${SRC} ${DATA}
        VERBATIM
    )
    # Generate the module registration code.
    set(REG_C ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${SYM}.c)
    configure_file(${MLUA_PATH}/core/lib_register.in.c ${REG_C})
    # Add the library.
    add_library(${TARGET} INTERFACE)
    target_sources(${TARGET} INTERFACE ${REG_C} PRIVATE ${DATA})
    target_link_libraries(${TARGET} INTERFACE mlua_core)
endfunction()
