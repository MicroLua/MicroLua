# Lua libraries for all .lua files.
file(GLOB modules *.lua)
foreach(path IN LISTS modules)
    cmake_path(GET path STEM LAST_ONLY module)
    mlua_add_lua_library(mlua_lib_${module} ${module} ${path})
endforeach()
