#include <stdbool.h>

#include "hardware/gpio.h"
#include "hardware/sync.h"
#include "hardware/structs/iobank0.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

static uint check_gpio(lua_State* ls, int index) {
    lua_Integer gpio = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= gpio && gpio < (lua_Integer)NUM_BANK0_GPIOS, index,
                  "invalid GPIO number");
    return gpio;
}

#if LIB_MLUA_MOD_MLUA_EVENT

#define EVENTS_SIZE ((NUM_BANK0_GPIOS + 7) / 8)

typedef struct IRQState {
    uint32_t pending[EVENTS_SIZE];
    uint32_t mask[EVENTS_SIZE];
    MLuaEvent irq_event;
} IRQState;

static IRQState irq_state[NUM_CORES];

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

static void __time_critical_func(handle_irq)(void) {
    uint core = get_core_num();
    IRQState* state = &irq_state[core];
    io_irq_ctrl_hw_t* irq_ctrl_base = core_irq_ctrl_base(core);
    bool notify = false;
    for (uint block = 0; block < MLUA_SIZE(state->pending); ++block) {
        uint32_t pending = irq_ctrl_base->ints[block] & state->mask[block];
        iobank0_hw->intr[block] = pending;  // Acknowledge
        state->pending[block] |= pending;
        notify = notify || pending != 0;

        // Disable active level-triggered events.
        pending &= LEVEL_EVENTS_MASK;
        hw_clear_bits(&irq_ctrl_base->inte[block], pending);
     }
     if (notify) mlua_event_set(state->irq_event);
}

static int mod_enable_irq(lua_State* ls) {
    char const* err = mlua_event_enable_irq(
        ls, &irq_state[get_core_num()].irq_event, IO_IRQ_BANK0, &handle_irq, 1,
        GPIO_RAW_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    if (err != NULL) return luaL_error(ls, "GPIO: %s", err);
    return 0;
}

static int try_events(lua_State* ls) {
    IRQState* state = &irq_state[get_core_num()];
    int cnt = lua_gettop(ls);
    bool active = false;
    for (int i = 1; i <= cnt; ++i) {
        uint gpio = lua_tointeger(ls, i);  // Arguments were checked before
        uint block = gpio / 8;
        int shift = 4 * (gpio % 8);
        uint32_t mask = 0xfu << shift;
        uint32_t save = save_and_disable_interrupts();
        uint32_t pending = state->pending[block];
        state->pending[block] &= ~mask;
        restore_interrupts(save);
        lua_Integer e = (pending & mask) >> shift;
        active = active || (e != 0);
        lua_pushinteger(ls, e);
    }
    if (active) return cnt;
    lua_pop(ls, cnt);
    return -1;
}

static int mod_wait_events(lua_State* ls) {
    int cnt = lua_gettop(ls);
    if (cnt == 0) return 0;

    // Enable level-triggered events once, to propagate their current state.
    uint core = get_core_num();
    IRQState* state = &irq_state[core];
    io_irq_ctrl_hw_t* irq_ctrl_base = core_irq_ctrl_base(core);
    for (int i = 1; i <= cnt; ++i) {
        uint gpio = check_gpio(ls, i);
        uint block = gpio / 8;
        uint32_t mask = state->mask[block]
                        & ((GPIO_IRQ_LEVEL_LOW | GPIO_IRQ_LEVEL_HIGH)
                           << 4 * (gpio % 8));
        hw_set_bits(&irq_ctrl_base->inte[block], mask);
    }

    return mlua_event_wait(ls, irq_state[get_core_num()].irq_event,
                           &try_events, 0);
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

int mod_set_irq_enabled(lua_State* ls) {
    uint gpio = check_gpio(ls, 1);
    uint32_t event_mask = luaL_checkinteger(ls, 2);
    bool enable = mlua_to_cbool(ls, 3);
#if LIB_MLUA_MOD_MLUA_EVENT
    IRQState* state = &irq_state[get_core_num()];
    uint block = gpio / 8;
    uint32_t mask = event_mask << 4 * (gpio % 8);
    uint32_t save = save_and_disable_interrupts();
    if (enable) {
        state->mask[block] |= mask;
    } else {
        state->mask[block] &= ~mask;
    }
    restore_interrupts(save);
#endif
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

static MLuaSym const module_syms[] = {
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
    // set_irq_callback: See enable_irq, wait_events
    // set_irq_enabled_with_callback: See enable_irq, wait_events
    MLUA_SYM_F(set_dormant_irq_enabled, mod_),
    MLUA_SYM_F(get_irq_event_mask, mod_),
    MLUA_SYM_F(acknowledge_irq, mod_),
    // gpio_add_raw_irq_handler_with_order_priority_masked: See enable_irq, wait_events
    // gpio_add_raw_irq_handler_with_order_priority: See enable_irq, wait_events
    // gpio_add_raw_irq_handler_masked: See enable_irq, wait_events
    // gpio_add_raw_irq_handler: See enable_irq, wait_events
    // gpio_remove_raw_irq_handler_masked: See enable_irq, wait_events
    // gpio_remove_raw_irq_handler: See enable_irq, wait_events
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
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM_F(enable_irq, mod_),
    MLUA_SYM_F(wait_events, mod_),
#endif
};

#if LIB_MLUA_MOD_MLUA_EVENT

static __attribute__((constructor)) void init(void) {
    for (uint core = 0; core < NUM_CORES; ++core) {
        irq_state[core].irq_event = MLUA_EVENT_UNSET;
    }
}

#endif  // LIB_MLUA_MOD_MLUA_EVENT

int luaopen_hardware_gpio(lua_State* ls) {
    mlua_event_require(ls);

    mlua_new_module(ls, 0, module_syms);
    return 1;
}
