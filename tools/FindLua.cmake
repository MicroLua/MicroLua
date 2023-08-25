if (NOT Lua_FOUND)
    include(ExternalProject)

    set(LUA_SOURCE_DIR "${MLUA_PATH}/tools/lua")
    set(LUA_BINARY_DIR "${CMAKE_BINARY_DIR}/lua")
    set(LuaBuild_TARGET LuaBuild)
    set(Lua_TARGET Lua)

    if(NOT TARGET "${LuaBuild_TARGET}")
        ExternalProject_Add("${LuaBuild_TARGET}"
            PREFIX lua
            SOURCE_DIR "${LUA_SOURCE_DIR}"
            BINARY_DIR "${LUA_BINARY_DIR}"
            CMAKE_ARGS
                "-DCMAKE_MAKE_PROGRAM:FILEPATH=${CMAKE_MAKE_PROGRAM}"
                "-DMLUA_PATH:PATH=${MLUA_PATH}"
                "-DMLUA_INT:STRING=${MLUA_INT}"
                "-DMLUA_FLOAT:STRING=${MLUA_FLOAT}"
            BUILD_ALWAYS 1  # Force dependency checking
            INSTALL_COMMAND ""
        )
    endif()

    set(Lua_EXECUTABLE "${LUA_BINARY_DIR}/lua")
    if(CMAKE_HOST_WIN32)
        set(Lua_EXECUTABLE "${Lua_EXECUTABLE}.exe")
    endif()
    if(NOT TARGET "${Lua_TARGET}")
        add_executable("${Lua_TARGET}" IMPORTED)
    endif()
    set_property(TARGET "${Lua_TARGET}"
        PROPERTY IMPORTED_LOCATION "${Lua_EXECUTABLE}"
    )
    add_dependencies("${Lua_TARGET}" "${LuaBuild_TARGET}")
    set(Lua_FOUND 1)
endif()
