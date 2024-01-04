// Copyright 2024 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/event.h"

spin_lock_t* mlua_event_spinlock;

lua_Integer mlua_event_parse_irq_priority(lua_State* ls, int arg,
                                          lua_Integer def) {
    switch (lua_type(ls, arg)) {
    case LUA_TNONE:
    case LUA_TNIL:
        return def;
    case LUA_TNUMBER:
        lua_Integer priority = luaL_checkinteger(ls, arg);
        luaL_argcheck(
            ls, priority <= PICO_SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY,
            arg, "invalid priority");
        return priority;
    default:
        return luaL_typeerror(ls, arg, "integer or nil");
    }
}

bool mlua_event_enable_irq_arg(lua_State* ls, int arg,
                               lua_Integer* priority) {
    if (lua_isboolean(ls, arg)) return lua_toboolean(ls, arg);
    *priority = mlua_event_parse_irq_priority(ls, arg, *priority);
    return true;
}

void mlua_event_set_irq_handler(uint irq, void (*handler)(void),
                                lua_Integer priority) {
    if (priority < 0) {
        irq_set_exclusive_handler(irq, handler);
    } else {
        irq_add_shared_handler(irq, handler, priority);
    }
}

bool mlua_event_enable_irq(lua_State* ls, MLuaEvent* ev, uint irq,
                           irq_handler_t handler, int index,
                           lua_Integer priority) {
    if (!mlua_event_enable_irq_arg(ls, index, &priority)) {  // Disable IRQ
        irq_set_enabled(irq, false);
        irq_remove_handler(irq, handler);
        mlua_event_disable(ls, ev);
        return true;
    }
    if (!mlua_event_enable(ls, ev)) return false;
    mlua_event_set_irq_handler(irq, handler, priority);
    irq_set_enabled(irq, true);
    return true;
}

static __attribute__((constructor)) void init(void) {
    mlua_event_spinlock = spin_lock_instance(next_striped_spin_lock_num());
}
