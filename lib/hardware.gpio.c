#include "hardware/gpio.h"

#include <stdbool.h>

#include "pico/platform.h"
#include "hardware/structs/iobank0.h"
#include "hardware/sync.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/signal.h"
#include "mlua/util.h"

struct IRQState {
    uint32_t events[(NUM_BANK0_GPIOS + 7) / 8];
    uint32_t masks[(NUM_BANK0_GPIOS + 7) / 8];
    SigNum callback_sig;
};

static struct IRQState irq_states[NUM_CORES];

#define LEVEL_EVENTS_MASK ( \
      ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 0) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 4) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 8) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 12) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 16) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 20) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 24) \
    | ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH) << 28))

io_irq_ctrl_hw_t* core_irq_ctrl_base(uint core) {
    return core == 1 ? &iobank0_hw->proc1_irq_ctrl
        : &iobank0_hw->proc0_irq_ctrl;
}

void irq_callback(uint gpio, uint32_t event_mask) {
    uint core = get_core_num();
    struct IRQState* irq_state = &irq_states[core];
    uint block = gpio / 8;
    uint32_t mask = event_mask << 4 * (gpio % 8);
    irq_state->events[block] |= mask;

    // Disable level-triggered events.
    mask &= LEVEL_EVENTS_MASK;
    io_irq_ctrl_hw_t* irq_ctrl_base = core_irq_ctrl_base(core);
    hw_clear_bits(&irq_ctrl_base->inte[block], mask);

    // Trigger signal handler.
    mlua_signal_set(irq_state->callback_sig, true);
}

int irq_callback_signal(lua_State* ls) {
    uint core = get_core_num();
    struct IRQState* irq_state = &irq_states[core];
    io_irq_ctrl_hw_t* irq_ctrl_base = core_irq_ctrl_base(core);

    // Dispatch events to the callback.
    for (uint block = 0; block < MLUA_SIZE(irq_state->events); ++block) {
        uint32_t save = save_and_disable_interrupts();
        uint32_t events8 = irq_state->events[block];
        irq_state->events[block] = 0;
        restore_interrupts(save);
        for (uint i = 0; events8 && i < 8; ++i) {
            uint32_t events = events8 & 0xf;
            if (events) {
                lua_pushvalue(ls, lua_upvalueindex(1));
                lua_pushinteger(ls, block * 8 + i);
                lua_pushinteger(ls, events);
                lua_call(ls, 2, 0);
            }
            events8 >>= 4;
        }

        // Re-enable level-trigered events.
        hw_set_bits(&irq_ctrl_base->inte[block],
                    irq_state->masks[block] & LEVEL_EVENTS_MASK);
    }
    return 0;
}

static lua_Integer check_gpio(lua_State* ls, int index) {
    lua_Integer gpio = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= gpio && gpio < (lua_Integer)NUM_BANK0_GPIOS, index,
                  "invalid GPIO number");
    return gpio;
}

int mod_set_irq_callback(lua_State* ls) {
    bool has_cb = mlua_signal_wrap_handler(ls, &irq_callback_signal, 1);
    SigNum* sig = &irq_states[get_core_num()].callback_sig;
    if (*sig < 0) {  // No callback is currently set
        if (!has_cb) return 0;
        mlua_signal_claim(ls, sig, 1);
        gpio_set_irq_callback(&irq_callback);
    } else if (has_cb) {  // An existing callback is being updated
        mlua_signal_set_handler(ls, *sig, 1);
    } else {  // An existing callback is being unset
        gpio_set_irq_callback(NULL);
        mlua_signal_unclaim(ls, sig);
    }
    return 0;
}

int mod_set_irq_enabled(lua_State* ls) {
    lua_Integer gpio = check_gpio(ls, 1);
    lua_Integer event_mask = luaL_checkinteger(ls, 2);
    bool enable = mlua_to_cbool(ls, 3);
    struct IRQState* irq_state = &irq_states[get_core_num()];
    uint block = gpio / 8;
    uint32_t mask = event_mask << 4 * (gpio % 8);
    if (enable) {
        irq_state->masks[block] |= mask;
    } else {
        irq_state->masks[block] &= ~mask;
    }
    gpio_set_irq_enabled(gpio, event_mask, enable);
    return 0;
}

// Not defined in hardware/gpio.h for some reason.
int gpio_get_pad(uint gpio);

MLUA_FUNC_1_1(mod_, gpio_, get_pad, lua_pushboolean, check_gpio)
MLUA_FUNC_0_2(mod_, gpio_, set_function, check_gpio, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_function, lua_pushinteger, check_gpio)
MLUA_FUNC_0_3(mod_, gpio_, set_pulls, check_gpio, mlua_to_cbool,
              mlua_to_cbool)
MLUA_FUNC_0_1(mod_, gpio_, pull_up, check_gpio)
MLUA_FUNC_1_1(mod_, gpio_, is_pulled_up, lua_pushboolean, check_gpio)
MLUA_FUNC_0_1(mod_, gpio_, pull_down, check_gpio)
MLUA_FUNC_1_1(mod_, gpio_, is_pulled_down, lua_pushboolean, check_gpio)
MLUA_FUNC_0_1(mod_, gpio_, disable_pulls, check_gpio)
MLUA_FUNC_0_2(mod_, gpio_, set_irqover, check_gpio, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_outover, check_gpio, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_inover, check_gpio, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_oeover, check_gpio, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_input_enabled, check_gpio, mlua_to_cbool)
MLUA_FUNC_0_2(mod_, gpio_, set_input_hysteresis_enabled, check_gpio,
              mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, is_input_hysteresis_enabled, lua_pushboolean,
              check_gpio)
MLUA_FUNC_0_2(mod_, gpio_, set_slew_rate, check_gpio, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_slew_rate, lua_pushinteger, check_gpio)
MLUA_FUNC_0_2(mod_, gpio_, set_drive_strength, check_gpio,
              luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get_drive_strength, lua_pushinteger,
              check_gpio)
MLUA_FUNC_0_3(mod_, gpio_, set_dormant_irq_enabled, check_gpio,
              luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, get_irq_event_mask, lua_pushinteger, check_gpio)
MLUA_FUNC_0_2(mod_, gpio_, acknowledge_irq, check_gpio, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, init, check_gpio)
MLUA_FUNC_0_1(mod_, gpio_, deinit, check_gpio)
MLUA_FUNC_0_1(mod_, gpio_, init_mask, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, gpio_, get, lua_pushboolean, check_gpio)
MLUA_FUNC_1_0(mod_, gpio_, get_all, lua_pushinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_mask, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, clr_mask, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, xor_mask, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, put_masked, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, put_all, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, put, check_gpio, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, get_out_level, lua_pushboolean, check_gpio)
MLUA_FUNC_0_1(mod_, gpio_, set_dir_out_masked, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_dir_in_masked, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_dir_masked, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_0_1(mod_, gpio_, set_dir_all_bits, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, gpio_, set_dir, check_gpio, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, gpio_, is_dir_out, lua_pushboolean, check_gpio)
MLUA_FUNC_1_1(mod_, gpio_, get_dir, lua_pushinteger, check_gpio)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(get_pad),
    X(set_function),
    X(get_function),
    X(set_pulls),
    X(pull_up),
    X(is_pulled_up),
    X(pull_down),
    X(is_pulled_down),
    X(disable_pulls),
    X(set_irqover),
    X(set_outover),
    X(set_inover),
    X(set_oeover),
    X(set_input_enabled),
    X(set_input_hysteresis_enabled),
    X(is_input_hysteresis_enabled),
    X(set_slew_rate),
    X(get_slew_rate),
    X(set_drive_strength),
    X(get_drive_strength),
    X(set_irq_enabled),
    X(set_irq_callback),
    // X(set_irq_enabled_with_callback),
    X(set_dormant_irq_enabled),
    X(get_irq_event_mask),
    X(acknowledge_irq),
    // X(add_raw_irq_handler_with_order_priority_masked),
    // X(add_raw_irq_handler_with_order_priority),
    // X(add_raw_irq_handler_masked),
    // X(add_raw_irq_handler),
    // X(remove_raw_irq_handler_masked),
    // X(remove_raw_irq_handler),
    X(init),
    X(deinit),
    X(init_mask),
    X(get),
    X(get_all),
    X(set_mask),
    X(clr_mask),
    X(xor_mask),
    X(put_masked),
    X(put_all),
    X(put),
    X(get_out_level),
    X(set_dir_out_masked),
    X(set_dir_in_masked),
    X(set_dir_masked),
    X(set_dir_all_bits),
    X(set_dir),
    X(is_dir_out),
    X(get_dir),
#undef X
#define X(n) MLUA_REG(integer, n, GPIO_ ## n)
    X(IN),
    X(OUT),
#undef X
#define X(n) MLUA_REG(integer, DRIVE_STRENGTH ## n, GPIO_DRIVE_STRENGTH ## n)
    X(_2MA),
    X(_4MA),
    X(_8MA),
    X(_12MA),
#undef X
#define X(n) MLUA_REG(integer, FUNC_ ## n, GPIO_FUNC_ ## n)
    X(XIP),
    X(SPI),
    X(UART),
    X(I2C),
    X(PWM),
    X(SIO),
    X(PIO0),
    X(PIO1),
    X(GPCK),
    X(USB),
    X(NULL),
#undef X
#define X(n) MLUA_REG(integer, IRQ_ ## n, GPIO_IRQ_ ## n)
    X(LEVEL_LOW),
    X(LEVEL_HIGH),
    X(EDGE_FALL),
    X(EDGE_RISE),
#undef X
#define X(n) MLUA_REG(integer, OVERRIDE_ ## n, GPIO_OVERRIDE_ ## n)
    X(NORMAL),
    X(INVERT),
    X(LOW),
    X(HIGH),
#undef X
#define X(n) MLUA_REG(integer, SLEW_RATE_ ## n, GPIO_SLEW_RATE_ ## n)
    X(SLOW),
    X(FAST),
#undef X
    {NULL},
};

int luaopen_hardware_gpio(lua_State* ls) {
    mlua_require(ls, "mlua.signal", false);

    // Initialize internal state.
    struct IRQState* irq_state = &irq_states[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    for (uint block = 0; block < MLUA_SIZE(irq_state->events); ++block) {
        irq_state->events[block] = 0;
        irq_state->masks[block] = 0;
    }
    irq_state->callback_sig = -1;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
