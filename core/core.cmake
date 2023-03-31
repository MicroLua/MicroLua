function(mlua_core_filenames VAR GLOB)
    file(GLOB paths lua/${GLOB})
    foreach(path IN LISTS paths)
        cmake_path(GET path FILENAME name)
        list(APPEND names ${name})
    endforeach()
    set(${VAR} ${names} PARENT_SCOPE)
endfunction()

function(mlua_core_copy FILENAMES DEST)
    foreach(name IN LISTS FILENAMES)
        configure_file(lua/${name} ${DEST}/${name} COPYONLY)
    endforeach()
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
    mlua_add_core_library(mlua_core_lib_${SYM})
    mlua_core_target_sources(mlua_core_lib_${SYM} ${SRC})
    target_link_libraries(mlua_core_lib_${SYM} INTERFACE mlua_core)
    if(${ARGC} GREATER 2)
        if(NOT "${ARGV2}" STREQUAL "NOREGISTER")
            message(FATAL_ERROR "Unknown parameter ${ARGV2}")
        endif()
    else()
        set(REG_C ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${SYM}.c)
        configure_file(${CMAKE_SOURCE_DIR}/core/lib_register.c.in ${REG_C})
        target_sources(mlua_core_lib_${SYM} INTERFACE ${REG_C})
    endif()
endfunction()

function(mlua_add_lua_library TARGET LIB SRC)
    string(REPLACE "." "_" SYM ${LIB})
    set(PATH ${SRC})
    add_library(${TARGET} INTERFACE)
    target_link_libraries(${TARGET} INTERFACE mlua_core)
    set(REG_C ${CMAKE_CURRENT_BINARY_DIR}/lib_register_${SYM}.c)
    configure_file(${CMAKE_SOURCE_DIR}/core/lib_register.c.in ${REG_C})
    set_source_files_properties(${REG_C} PROPERTIES OBJECT_DEPENDS ${SRC})
    target_sources(${TARGET} INTERFACE ${REG_C})
endfunction()
