// Copyright 2023 Remy Blank <remy@c-space.org>
// SPDX-License-Identifier: MIT

#include "mlua/hardware.gpio.h"

#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/iobank0.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/module.h"
#include "mlua/thread.h"
#include "mlua/util.h"

#if LIB_MLUA_MOD_MLUA_THREAD

#define EVENTS_SIZE ((NUM_BANK0_GPIOS + 7) / 8)

typedef struct IRQState {
    uint32_t pending[EVENTS_SIZE];
    uint32_t mask[EVENTS_SIZE];
    MLuaEvent irq_event;
} IRQState;

static IRQState irq_state[NUM_CORES];

#define LEVEL_MASK (GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)
#define EDGE_MASK (GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE)
#define LEVEL_MASK_BLOCK ( \
      (LEVEL_MASK << 0) | (LEVEL_MASK << 4) | (LEVEL_MASK << 8) \
    | (LEVEL_MASK << 12) | (LEVEL_MASK << 16) | (LEVEL_MASK << 20) \
    | (LEVEL_MASK << 24) | (LEVEL_MASK << 28))
#define EDGE_MASK_BLOCK ( \
      (EDGE_MASK << 0) | (EDGE_MASK << 4) | (EDGE_MASK << 8) \
    | (EDGE_MASK << 12) | (EDGE_MASK << 16) | (EDGE_MASK << 20) \
    | (EDGE_MASK << 24) | (EDGE_MASK << 28))

io_bank0_irq_ctrl_hw_t* core_irq_ctrl_base(uint core) {
    return core == 1 ? &iobank0_hw->proc1_irq_ctrl
        : &iobank0_hw->proc0_irq_ctrl;
}

static void __time_critical_func(handle_gpio_irq)(void) {
    uint core = get_core_num();
    IRQState* state = &irq_state[core];
    io_bank0_irq_ctrl_hw_t* irq_ctrl_base = core_irq_ctrl_base(core);
    bool notify = false;
    for (uint block = 0; block < MLUA_SIZE(state->pending); ++block) {
        uint32_t pending = irq_ctrl_base->ints[block] & state->mask[block];
        iobank0_hw->intr[block] = pending;  // Acknowledge
        state->pending[block] |= pending & EDGE_MASK_BLOCK;
        notify = notify || pending != 0;

        // Disable active level-triggered events.
        hw_clear_bits(&irq_ctrl_base->inte[block], pending & LEVEL_MASK_BLOCK);
     }
     if (notify) mlua_event_set(&state->irq_event);
}

static int handle_irq_event_1(lua_State* ls, int status, lua_KContext ctx);

static int handle_irq_event(lua_State* ls) {
    lua_pushinteger(ls, (uint)-1);  // block
    lua_pushinteger(ls, 0);  // pending
    lua_rawgetp(ls, LUA_REGISTRYINDEX, &irq_state[get_core_num()].pending[0]);
    return handle_irq_event_1(ls, LUA_OK, lua_absindex(ls, -3));
}

static int handle_irq_event_1(lua_State* ls, int status, lua_KContext ctx) {
    uint core = get_core_num();
    IRQState* state = &irq_state[core];
    io_bank0_irq_ctrl_hw_t* irq_ctrl_base = core_irq_ctrl_base(core);
    uint block = lua_tointeger(ls, ctx);
    uint32_t pending = lua_tointeger(ls, ctx + 1);
    for (;;) {
        while (pending == 0) {
            ++block;
            if (block > 0) {  // Re-enable level-triggered events
                hw_set_bits(&irq_ctrl_base->inte[block - 1],
                            state->mask[block - 1] & LEVEL_MASK_BLOCK);
            }
            if (block >= MLUA_SIZE(state->pending)) return 0;
            lua_pushinteger(ls, block);  // Update block
            lua_replace(ls, ctx);
            pending = iobank0_hw->intr[block];  // Raw state
            uint32_t save = save_and_disable_interrupts();
            pending |= state->pending[block];  // Mix in stored edge state
            state->pending[block] = 0;  // Acknowledge
            restore_interrupts(save);
            pending &= state->mask[block];
        }

        // Call the IRQ callback.
        int shift = MLUA_CTZ(pending) & ~3;
        lua_pushvalue(ls, ctx + 2);  // handler
        lua_pushinteger(ls, 8 * block + shift / 4);  // gpio
        lua_pushinteger(ls, (pending >> shift) & 0xfu);  // event_mask
        pending &= ~(0xfu << shift);
        lua_pushinteger(ls, pending);  // Update pending
        lua_replace(ls, ctx + 1);
        lua_callk(ls, 2, 0, ctx, &handle_irq_event_1);
    }
}

static int irq_handler_done(lua_State* ls) {
    IRQState* state = &irq_state[get_core_num()];
    lua_pushnil(ls);
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &state->pending[0]);
    irq_remove_handler(IO_IRQ_BANK0, &handle_gpio_irq);
    mlua_event_disable(ls, &state->irq_event);
    return 0;
}

static int mod_set_irq_callback(lua_State* ls) {
    if (lua_isnone(ls, 1)) return 0;  // No argument => no change
    IRQState* state = &irq_state[get_core_num()];
    MLuaEvent* event = &state->irq_event;
    if (lua_isnil(ls, 1)) {  // Nil callback, kill the handler thread
        mlua_event_stop_handler(ls, event);
        return 0;
    }

    // Set the IRQ handler.
    bool enabled = mlua_event_enable(ls, event);
    if (enabled) {
        irq_add_shared_handler(IO_IRQ_BANK0, &handle_gpio_irq,
                               GPIO_IRQ_CALLBACK_ORDER_PRIORITY);
    }

    // Set the callback handler.
    lua_settop(ls, 1);  // handler
    lua_rawsetp(ls, LUA_REGISTRYINDEX, &state->pending[0]);

    // Start the event handler thread if it isn't already running.
    if (!enabled) {
        mlua_event_push_handler_thread(ls, event);
        return 1;
    }
    lua_pushcfunction(ls, &handle_irq_event);
    lua_pushcfunction(ls, &irq_handler_done);
    return mlua_event_handle(ls, event, &mlua_cont_return, 1);
}

static int mod_set_irq_enabled(lua_State* ls);
static int mod_set_irq_enabled_with_callback_1(lua_State* ls, int status,
                                               lua_KContext ctx);

static int mod_set_irq_enabled_with_callback(lua_State* ls) {
    lua_pushcfunction(ls, &mod_set_irq_enabled);
    lua_pushvalue(ls, 1);
    lua_pushvalue(ls, 2);
    lua_pushvalue(ls, 3);
    lua_call(ls, 3, 0);
    lua_KContext ctx = 0;
    if (!lua_isnone(ls, 4)) {
        ctx = 1;
        lua_pushcfunction(ls, &mod_set_irq_callback);
        lua_pushvalue(ls, 4);
        lua_callk(ls, 1, 1, ctx, &mod_set_irq_enabled_with_callback_1);
    }
    return mod_set_irq_enabled_with_callback_1(ls, LUA_OK, ctx);
}

static int mod_set_irq_enabled_with_callback_1(lua_State* ls, int status,
                                               lua_KContext ctx) {
    if (mlua_to_cbool(ls, 3)) irq_set_enabled(IO_IRQ_BANK0, true);
    return ctx;
}

#endif  // LIB_MLUA_MOD_MLUA_THREAD

static void _gpio_set_irq_enabled(uint gpio, uint32_t events, bool enabled) {
    // BUG(pico-sdk): We can't use gpio_set_irq_enabled() because it asserts
    // conditions that aren't fulfilled in certain use cases.
    gpio_acknowledge_irq(gpio, events);
    io_bank0_irq_ctrl_hw_t* hw = get_core_num() ?
                                 &io_bank0_hw->proc1_irq_ctrl :
                                 &io_bank0_hw->proc0_irq_ctrl;
    io_rw_32* inte = &hw->inte[gpio / 8];
    events <<= 4 * (gpio % 8);
    if (enabled) {
        hw_set_bits(inte, events);
    } else {
        hw_clear_bits(inte, events);
    }
}

typedef void (*IRQEnabler)(uint, uint32_t, bool);

static int set_irq_enabled(lua_State* ls, IRQEnabler set_enabled) {
    uint gpio = mlua_check_gpio(ls, 1);
    uint32_t event_mask = luaL_checkinteger(ls, 2);
    bool enabled = mlua_to_cbool(ls, 3);
#if LIB_MLUA_MOD_MLUA_THREAD
    IRQState* state = &irq_state[get_core_num()];
    uint block = gpio / 8;
    uint32_t mask = event_mask << 4 * (gpio % 8);
    uint32_t save = save_and_disable_interrupts();
    // Acknowledge before enabling. gpio_set_irq_enabled() does the same.
    state->pending[block] &= ~mask;
    if (enabled) {
        state->mask[block] |= mask;
    } else {
        state->mask[block] &= ~mask;
    }
    set_enabled(gpio, event_mask, enabled);
    restore_interrupts(save);
#else
    set_enabled(gpio, event_mask, enabled);
#endif
    return 0;
}

static int mod_set_irq_enabled(lua_State* ls) {
    return set_irq_enabled(ls, &_gpio_set_irq_enabled);
}

static int mod_set_dormant_irq_enabled(lua_State* ls) {
    return set_irq_enabled(ls, &gpio_set_dormant_irq_enabled);
}

static int mod_get_irq_event_mask(lua_State* ls) {
    uint gpio = mlua_check_gpio(ls, 1);
    uint block = gpio / 8;
    uint32_t pending = iobank0_hw->intr[block];  // Raw state
#if LIB_MLUA_MOD_MLUA_THREAD
    IRQState* state = &irq_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    pending |= state->pending[block];  // Mix in stored edge state
    restore_interrupts(save);
    pending &= state->mask[block];
#endif
    lua_pushinteger(ls, (pending >> 4 * (gpio % 8)) & 0xfu);
    return 1;
}

static int mod_acknowledge_irq(lua_State* ls) {
    uint gpio = mlua_check_gpio(ls, 1);
    uint32_t event_mask = luaL_checkinteger(ls, 2);
#if LIB_MLUA_MOD_MLUA_THREAD
    IRQState* state = &irq_state[get_core_num()];
    uint block = gpio / 8;
    uint32_t mask = event_mask << 4 * (gpio % 8);
    uint32_t save = save_and_disable_interrupts();
    state->pending[block] &= ~mask;
    iobank0_hw->intr[block] = mask;  // Acknowledge
    restore_interrupts(save);
#else
    gpio_acknowledge_irq(gpio, event_mask);
#endif
    return 0;
}

// Not declared in hardware/gpio.h for some reason.
int gpio_get_pad(uint gpio);

static int mod_get_status(lua_State* ls) {
    uint gpio = mlua_check_gpio(ls, 1);
    lua_pushinteger(ls, iobank0_hw->io[gpio].status);
    return 1;
}

MLUA_FUNC_R1(mod_, gpio_, get_pad, lua_pushboolean, mlua_check_gpio)
MLUA_FUNC_V2(mod_, gpio_, set_function, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_R1(mod_, gpio_, get_function, lua_pushinteger, mlua_check_gpio)
MLUA_FUNC_V3(mod_, gpio_, set_pulls, mlua_check_gpio, mlua_to_cbool,
             mlua_to_cbool)
MLUA_FUNC_V1(mod_, gpio_, pull_up, mlua_check_gpio)
MLUA_FUNC_R1(mod_, gpio_, is_pulled_up, lua_pushboolean, mlua_check_gpio)
MLUA_FUNC_V1(mod_, gpio_, pull_down, mlua_check_gpio)
MLUA_FUNC_R1(mod_, gpio_, is_pulled_down, lua_pushboolean, mlua_check_gpio)
MLUA_FUNC_V1(mod_, gpio_, disable_pulls, mlua_check_gpio)
MLUA_FUNC_V2(mod_, gpio_, set_irqover, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, set_outover, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, set_inover, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, set_oeover, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, set_input_enabled, mlua_check_gpio, mlua_to_cbool)
MLUA_FUNC_V2(mod_, gpio_, set_input_hysteresis_enabled, mlua_check_gpio,
             mlua_to_cbool)
MLUA_FUNC_R1(mod_, gpio_, is_input_hysteresis_enabled, lua_pushboolean,
             mlua_check_gpio)
MLUA_FUNC_V2(mod_, gpio_, set_slew_rate, mlua_check_gpio, luaL_checkinteger)
MLUA_FUNC_R1(mod_, gpio_, get_slew_rate, lua_pushinteger, mlua_check_gpio)
MLUA_FUNC_V2(mod_, gpio_, set_drive_strength, mlua_check_gpio,
             luaL_checkinteger)
MLUA_FUNC_R1(mod_, gpio_, get_drive_strength, lua_pushinteger, mlua_check_gpio)
MLUA_FUNC_V1(mod_, gpio_, init, mlua_check_gpio)
MLUA_FUNC_V1(mod_, gpio_, deinit, mlua_check_gpio)
MLUA_FUNC_V1(mod_, gpio_, init_mask, luaL_checkinteger)
MLUA_FUNC_R1(mod_, gpio_, get, lua_pushboolean, mlua_check_gpio)
MLUA_FUNC_R0(mod_, gpio_, get_all, lua_pushinteger)
MLUA_FUNC_V1(mod_, gpio_, set_mask, luaL_checkinteger)
MLUA_FUNC_V1(mod_, gpio_, clr_mask, luaL_checkinteger)
MLUA_FUNC_V1(mod_, gpio_, xor_mask, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, put_masked, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_V1(mod_, gpio_, put_all, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, put, mlua_check_gpio, mlua_to_cbool)
MLUA_FUNC_R1(mod_, gpio_, get_out_level, lua_pushboolean, mlua_check_gpio)
MLUA_FUNC_V1(mod_, gpio_, set_dir_out_masked, luaL_checkinteger)
MLUA_FUNC_V1(mod_, gpio_, set_dir_in_masked, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, set_dir_masked, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_V1(mod_, gpio_, set_dir_all_bits, luaL_checkinteger)
MLUA_FUNC_V2(mod_, gpio_, set_dir, mlua_check_gpio, mlua_to_cbool)
MLUA_FUNC_R1(mod_, gpio_, is_dir_out, lua_pushboolean, mlua_check_gpio)
MLUA_FUNC_R1(mod_, gpio_, get_dir, lua_pushinteger, mlua_check_gpio)

MLUA_SYMBOLS(module_syms) = {
    MLUA_SYM_V(FUNC_XIP, integer, GPIO_FUNC_XIP),
    MLUA_SYM_V(FUNC_SPI, integer, GPIO_FUNC_SPI),
    MLUA_SYM_V(FUNC_UART, integer, GPIO_FUNC_UART),
    MLUA_SYM_V(FUNC_I2C, integer, GPIO_FUNC_I2C),
    MLUA_SYM_V(FUNC_PWM, integer, GPIO_FUNC_PWM),
    MLUA_SYM_V(FUNC_SIO, integer, GPIO_FUNC_SIO),
    MLUA_SYM_V(FUNC_PIO0, integer, GPIO_FUNC_PIO0),
    MLUA_SYM_V(FUNC_PIO1, integer, GPIO_FUNC_PIO1),
    MLUA_SYM_V(FUNC_GPCK, integer, GPIO_FUNC_GPCK),
    MLUA_SYM_V(FUNC_USB, integer, GPIO_FUNC_USB),
    MLUA_SYM_V(FUNC_NULL, integer, GPIO_FUNC_NULL),
    MLUA_SYM_V(IN, integer, GPIO_IN),
    MLUA_SYM_V(OUT, integer, GPIO_OUT),
    MLUA_SYM_V(IRQ_LEVEL_LOW, integer, GPIO_IRQ_LEVEL_LOW),
    MLUA_SYM_V(IRQ_LEVEL_HIGH, integer, GPIO_IRQ_LEVEL_HIGH),
    MLUA_SYM_V(IRQ_EDGE_FALL, integer, GPIO_IRQ_EDGE_FALL),
    MLUA_SYM_V(IRQ_EDGE_RISE, integer, GPIO_IRQ_EDGE_RISE),
    MLUA_SYM_V(OVERRIDE_NORMAL, integer, GPIO_OVERRIDE_NORMAL),
    MLUA_SYM_V(OVERRIDE_INVERT, integer, GPIO_OVERRIDE_INVERT),
    MLUA_SYM_V(OVERRIDE_LOW, integer, GPIO_OVERRIDE_LOW),
    MLUA_SYM_V(OVERRIDE_HIGH, integer, GPIO_OVERRIDE_HIGH),
    MLUA_SYM_V(SLEW_RATE_SLOW, integer, GPIO_SLEW_RATE_SLOW),
    MLUA_SYM_V(SLEW_RATE_FAST, integer, GPIO_SLEW_RATE_FAST),
    MLUA_SYM_V(DRIVE_STRENGTH_2MA, integer, GPIO_DRIVE_STRENGTH_2MA),
    MLUA_SYM_V(DRIVE_STRENGTH_4MA, integer, GPIO_DRIVE_STRENGTH_4MA),
    MLUA_SYM_V(DRIVE_STRENGTH_8MA, integer, GPIO_DRIVE_STRENGTH_8MA),
    MLUA_SYM_V(DRIVE_STRENGTH_12MA, integer, GPIO_DRIVE_STRENGTH_12MA),

    MLUA_SYM_F(get_pad, mod_),
    MLUA_SYM_F(get_status, mod_),
    MLUA_SYM_F(set_function, mod_),
    MLUA_SYM_F(get_function, mod_),
    MLUA_SYM_F(set_pulls, mod_),
    MLUA_SYM_F(pull_up, mod_),
    MLUA_SYM_F(is_pulled_up, mod_),
    MLUA_SYM_F(pull_down, mod_),
    MLUA_SYM_F(is_pulled_down, mod_),
    MLUA_SYM_F(disable_pulls, mod_),
    MLUA_SYM_F(set_irqover, mod_),
    MLUA_SYM_F(set_outover, mod_),
    MLUA_SYM_F(set_inover, mod_),
    MLUA_SYM_F(set_oeover, mod_),
    MLUA_SYM_F(set_input_enabled, mod_),
    MLUA_SYM_F(set_input_hysteresis_enabled, mod_),
    MLUA_SYM_F(is_input_hysteresis_enabled, mod_),
    MLUA_SYM_F(set_slew_rate, mod_),
    MLUA_SYM_F(get_slew_rate, mod_),
    MLUA_SYM_F(set_drive_strength, mod_),
    MLUA_SYM_F(get_drive_strength, mod_),
    MLUA_SYM_F(set_irq_enabled, mod_),
    MLUA_SYM_F_THREAD(set_irq_callback, mod_),
    MLUA_SYM_F_THREAD(set_irq_enabled_with_callback, mod_),
    MLUA_SYM_F(set_dormant_irq_enabled, mod_),
    MLUA_SYM_F(get_irq_event_mask, mod_),
    MLUA_SYM_F(acknowledge_irq, mod_),
    // gpio_add_raw_irq_handler_with_order_priority_masked: Not useful with thread-based handlers
    // gpio_add_raw_irq_handler_with_order_priority: Not useful with thread-based handlers
    // TODO: MLUA_SYM_F(add_raw_irq_handler_masked, mod_),
    // TODO: MLUA_SYM_F(add_raw_irq_handler, mod_),
    // TODO: MLUA_SYM_F(remove_raw_irq_handler_masked, mod_),
    // TODO: MLUA_SYM_F(remove_raw_irq_handler, mod_),
    MLUA_SYM_F(init, mod_),
    MLUA_SYM_F(deinit, mod_),
    MLUA_SYM_F(init_mask, mod_),
    MLUA_SYM_F(get, mod_),
    MLUA_SYM_F(get_all, mod_),
    MLUA_SYM_F(set_mask, mod_),
    MLUA_SYM_F(clr_mask, mod_),
    MLUA_SYM_F(xor_mask, mod_),
    MLUA_SYM_F(put_masked, mod_),
    MLUA_SYM_F(put_all, mod_),
    MLUA_SYM_F(put, mod_),
    MLUA_SYM_F(get_out_level, mod_),
    MLUA_SYM_F(set_dir_out_masked, mod_),
    MLUA_SYM_F(set_dir_in_masked, mod_),
    MLUA_SYM_F(set_dir_masked, mod_),
    MLUA_SYM_F(set_dir_all_bits, mod_),
    MLUA_SYM_F(set_dir, mod_),
    MLUA_SYM_F(is_dir_out, mod_),
    MLUA_SYM_F(get_dir, mod_),
};

MLUA_OPEN_MODULE(hardware.gpio) {
    mlua_thread_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
