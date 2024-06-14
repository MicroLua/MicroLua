# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

set(test_pattern "^mlua_test((-[^_]+)?)_.*$")

function(mlua_test_suffix TARGET VAR)
    string(REGEX REPLACE "${test_pattern}" "\\1" suffix "${TARGET}")
    set("${VAR}" "${suffix}" PARENT_SCOPE)
endfunction()

# Executable: standalone binary
mlua_add_executable(microlua)
target_compile_definitions(microlua PRIVATE
    MLUA_SYMBOL_HASH_DEBUG=0
)
mlua_target_config(microlua
)
target_link_libraries(microlua PRIVATE
    mlua_mod_mlua.stdio
)
mlua_list_targets(modules
    DIRS "${MLUA_PATH}/core" "${MLUA_PATH}/lib"
    INCLUDE "^mlua_mod_.*$"
    EXCLUDE "^mlua_mod_.*_headers$"
            "^mlua_mod_mlua\\.testing\\..*$"
)
list(SORT modules COMPARE NATURAL)
target_link_libraries(microlua PRIVATE ${modules})
mlua_platform_bin_standalone(microlua)

# Executables: unit tests
mlua_list_targets(all_tests
    DIRS "${MLUA_PATH}/core" "${MLUA_PATH}/lib"
    INCLUDE "${test_pattern}"
    EXCLUDE "^.*_headers$"
)
list(SORT all_tests COMPARE NATURAL)
set(suffixes "@")  # Necessary for adding empty values
foreach(test IN LISTS all_tests)
    mlua_test_suffix("${test}" suffix)
    list(APPEND suffixes "${suffix}")
endforeach()
list(REMOVE_DUPLICATES suffixes)
list(SORT suffixes COMPARE NATURAL)
foreach(suffix IN LISTS suffixes)
    if("${suffix}" STREQUAL "@" OR "${suffix}" STREQUAL "-ALL")
        continue()
    endif()
    set(target "mlua_tests${suffix}")
    message("Test binary: ${target}")
    mlua_add_executable("${target}")
    target_compile_definitions("${target}" PRIVATE
        MLUA_ALLOC_STATS=1
        MLUA_THREAD_STATS=1
        MLUA_MAIN_SHUTDOWN=1
        MLUA_MAIN_TRACEBACK=1
        MLUA_MAIN_MODULE=mlua.testing
        MLUA_SYMBOL_HASH_DEBUG=0
    )
    if("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
        target_compile_definitions("${target}" PRIVATE
            LUA_USE_APICHECK=1
            LUAI_ASSERT=1
        )
    endif()
    mlua_target_config("${target}"
        HASH_SYMBOL_TABLES:integer=MLUA_HASH_SYMBOL_TABLES
        a:integer=1
        b:string="test"
    )
    target_link_libraries("${target}" PRIVATE
        mlua_mod_mlua.stdio
        mlua_mod_mlua.testing
        mlua_mod_mlua.thread
    )
    set(tests)
    foreach(test IN LISTS all_tests)
        mlua_test_suffix("${test}" ts)
        if("${ts}" STREQUAL "${suffix}" OR "${ts}" STREQUAL "-ALL")
            list(APPEND tests "${test}")
        endif()
    endforeach()
    target_link_libraries("${target}" PRIVATE ${tests})
    mlua_platform_bin_tests("${target}" "${suffix}")
endforeach()
