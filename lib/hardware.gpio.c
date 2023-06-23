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

static MLuaReg const module_regs[] = {
#define MLUA_SYM(n) MLUA_REG(integer, n, GPIO_ ## n)
    MLUA_SYM(FUNC_XIP),
    MLUA_SYM(FUNC_SPI),
    MLUA_SYM(FUNC_UART),
    MLUA_SYM(FUNC_I2C),
    MLUA_SYM(FUNC_PWM),
    MLUA_SYM(FUNC_SIO),
    MLUA_SYM(FUNC_PIO0),
    MLUA_SYM(FUNC_PIO1),
    MLUA_SYM(FUNC_GPCK),
    MLUA_SYM(FUNC_USB),
    MLUA_SYM(FUNC_NULL),
    MLUA_SYM(IN),
    MLUA_SYM(OUT),
    MLUA_SYM(IRQ_LEVEL_LOW),
    MLUA_SYM(IRQ_LEVEL_HIGH),
    MLUA_SYM(IRQ_EDGE_FALL),
    MLUA_SYM(IRQ_EDGE_RISE),
    MLUA_SYM(OVERRIDE_NORMAL),
    MLUA_SYM(OVERRIDE_INVERT),
    MLUA_SYM(OVERRIDE_LOW),
    MLUA_SYM(OVERRIDE_HIGH),
    MLUA_SYM(SLEW_RATE_SLOW),
    MLUA_SYM(SLEW_RATE_FAST),
    MLUA_SYM(DRIVE_STRENGTH_2MA),
    MLUA_SYM(DRIVE_STRENGTH_4MA),
    MLUA_SYM(DRIVE_STRENGTH_8MA),
    MLUA_SYM(DRIVE_STRENGTH_12MA),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(get_pad),
    MLUA_SYM(set_function),
    MLUA_SYM(get_function),
    MLUA_SYM(set_pulls),
    MLUA_SYM(pull_up),
    MLUA_SYM(is_pulled_up),
    MLUA_SYM(pull_down),
    MLUA_SYM(is_pulled_down),
    MLUA_SYM(disable_pulls),
    MLUA_SYM(set_irqover),
    MLUA_SYM(set_outover),
    MLUA_SYM(set_inover),
    MLUA_SYM(set_oeover),
    MLUA_SYM(set_input_enabled),
    MLUA_SYM(set_input_hysteresis_enabled),
    MLUA_SYM(is_input_hysteresis_enabled),
    MLUA_SYM(set_slew_rate),
    MLUA_SYM(get_slew_rate),
    MLUA_SYM(set_drive_strength),
    MLUA_SYM(get_drive_strength),
    MLUA_SYM(set_irq_enabled),
    // set_irq_callback: See enable_irq, wait_events
    // set_irq_enabled_with_callback: See enable_irq, wait_events
    MLUA_SYM(set_dormant_irq_enabled),
    MLUA_SYM(get_irq_event_mask),
    MLUA_SYM(acknowledge_irq),
    // gpio_add_raw_irq_handler_with_order_priority_masked: See enable_irq, wait_events
    // gpio_add_raw_irq_handler_with_order_priority: See enable_irq, wait_events
    // gpio_add_raw_irq_handler_masked: See enable_irq, wait_events
    // gpio_add_raw_irq_handler: See enable_irq, wait_events
    // gpio_remove_raw_irq_handler_masked: See enable_irq, wait_events
    // gpio_remove_raw_irq_handler: See enable_irq, wait_events
    MLUA_SYM(init),
    MLUA_SYM(deinit),
    MLUA_SYM(init_mask),
    MLUA_SYM(get),
    MLUA_SYM(get_all),
    MLUA_SYM(set_mask),
    MLUA_SYM(clr_mask),
    MLUA_SYM(xor_mask),
    MLUA_SYM(put_masked),
    MLUA_SYM(put_all),
    MLUA_SYM(put),
    MLUA_SYM(get_out_level),
    MLUA_SYM(set_dir_out_masked),
    MLUA_SYM(set_dir_in_masked),
    MLUA_SYM(set_dir_masked),
    MLUA_SYM(set_dir_all_bits),
    MLUA_SYM(set_dir),
    MLUA_SYM(is_dir_out),
    MLUA_SYM(get_dir),
#if LIB_MLUA_MOD_MLUA_EVENT
    MLUA_SYM(enable_irq),
    MLUA_SYM(wait_events),
#endif
#undef MLUA_SYM
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

    // Create the module.
    mlua_new_table(ls, module_regs);
    return 1;
}
