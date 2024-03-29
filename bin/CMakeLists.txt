# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

# Executable: standalone binary
mlua_add_executable(mlua_standalone)
target_compile_definitions(mlua_standalone PRIVATE
    MLUA_SYMBOL_HASH_DEBUG=0
)
mlua_target_config(mlua_standalone
)
target_link_libraries(mlua_standalone PRIVATE
    mlua_mod_mlua.stdio
)
mlua_list_targets(modules
    DIRS "${MLUA_PATH}/core" "${MLUA_PATH}/lib"
    INCLUDE "^mlua_mod_.*$"
    EXCLUDE "^mlua_mod_.*_headers$"
            "^mlua_mod_mlua\\.testing\\..*$"
)
list(SORT modules COMPARE NATURAL)
target_link_libraries(mlua_standalone PRIVATE ${modules})
mlua_platform_bin_standalone(mlua_standalone)

# Executable: unit tests
mlua_add_executable(mlua_tests)
target_compile_definitions(mlua_tests PRIVATE
    MLUA_MAIN_MODULE=mlua.testing
    MLUA_SYMBOL_HASH_DEBUG=0
)
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
    target_compile_definitions(mlua_tests PRIVATE
        LUA_USE_APICHECK=1
        LUAI_ASSERT=1
    )
endif()
mlua_target_config(mlua_tests
    HASH_SYMBOL_TABLES:integer=MLUA_HASH_SYMBOL_TABLES
    I2C_MASTER_SCL:integer=5
    I2C_MASTER_SDA:integer=4
    I2C_SLAVE_SCL:integer=7
    I2C_SLAVE_SDA:integer=6
    a:integer=1
    b:string="test"
)
target_link_libraries(mlua_tests PRIVATE
    mlua_mod_mlua.stdio
    mlua_mod_mlua.testing
)
mlua_list_targets(tests
    DIRS "${MLUA_PATH}/core" "${MLUA_PATH}/lib"
    INCLUDE "^mlua_test_.*$"
)
list(SORT tests COMPARE NATURAL)
target_link_libraries(mlua_tests PRIVATE ${tests})
mlua_platform_bin_tests(mlua_tests)
