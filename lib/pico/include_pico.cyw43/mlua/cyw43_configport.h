// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#ifndef _MLUA_LIB_PICO_PICO_CYW43_CONFIGPORT_H
#define _MLUA_LIB_PICO_PICO_CYW43_CONFIGPORT_H

// Avoid the default dependency on lwIP, defined in cyw43_config.h.
#ifndef CYW43_LWIP
#define CYW43_LWIP 0
#endif

#include <cyw43_configport.h>

#endif
