# Copyright 2023 Remy Blank <remy@c-space.org>
# SPDX-License-Identifier: MIT

mlua_add_tool(mlua_tool_gen
    "${MLUA_PATH}/tools/gen"
    "${CMAKE_BINARY_DIR}/tools/gen/gen")
