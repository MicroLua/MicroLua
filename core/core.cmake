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

function(mlua_core_headers_library TARGET)
    add_library(${TARGET}_headers INTERFACE)
    target_include_directories(${TARGET}_headers INTERFACE
        ${CMAKE_CURRENT_BINARY_DIR}/include
    )
endfunction()

function(mlua_core_library TARGET)
    mlua_core_headers_library(${TARGET})
    pico_add_impl_library(${TARGET})
endfunction()

function(mlua_core_target_sources TARGET)
    foreach(name IN LISTS ARGN)
        target_sources(${TARGET} INTERFACE
            ${CMAKE_CURRENT_BINARY_DIR}/src/${name}
        )
    endforeach()
endfunction()

function(mlua_core_lib_library LIB SRC)
    mlua_core_library(mlua_core_lib_${LIB})
    mlua_core_target_sources(mlua_core_lib_${LIB} ${SRC})
    target_link_libraries(mlua_core_lib_${LIB} INTERFACE mlua_core)
endfunction()

function(mlua_sources TARGET)
    set(scope PRIVATE)
    foreach(PATH IN LISTS ARGN)
        string(REGEX MATCH "^(PRIVATE|PUBLIC|INTERFACE)$" kw "${PATH}")
        if("${kw}" STREQUAL "")
            get_filename_component(NAME ${PATH} NAME)
            set(PATH_C ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.c)
            configure_file(${CMAKE_SOURCE_DIR}/core/file_data.c.in ${PATH_C})
            set_source_files_properties(${PATH_C} PROPERTIES
                OBJECT_DEPENDS ${PATH}
            )
            target_sources(${TARGET} ${scope} ${PATH_C})
        else()
            set(scope "${kw}")
        endif()
    endforeach()
endfunction()
