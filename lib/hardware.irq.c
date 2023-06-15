#include <string.h>

#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/event.h"
#include "mlua/util.h"

static bool is_user_irq(uint irq) {
    return FIRST_USER_IRQ <= irq && irq < FIRST_USER_IRQ + NUM_USER_IRQS;
}

static uint check_irq(lua_State* ls, int index) {
    lua_Integer irq = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= irq && irq < (lua_Integer)NUM_IRQS, index,
                  "unsupported IRQ number");
    return irq;
 }

static uint check_user_irq(lua_State* ls, int index) {
    lua_Integer irq = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, is_user_irq(irq), index, "unsupported user IRQ number");
    return irq;
}

struct IRQState {
    Event events[NUM_USER_IRQS];
    uint8_t pending;
};

static_assert(NUM_USER_IRQS <= 8 * sizeof(uint8_t),
              "pending bitmask too small");

static struct IRQState uirq_state[NUM_CORES];

static void __time_critical_func(handle_user_irq)(void) {
    uint num = __get_current_exception() - VTABLE_FIRST_IRQ - FIRST_USER_IRQ;
    struct IRQState* state = &uirq_state[get_core_num()];
    state->pending |= 1u << num;
    mlua_event_set(state->events[num], true);
}

static int mod_enable_user_irq(lua_State* ls) {
    uint irq = check_user_irq(ls, 1);
    mlua_event_enable_irq(
        ls, &uirq_state[get_core_num()].events[irq - FIRST_USER_IRQ], irq,
        &handle_user_irq, 2, -1);
    return 0;
}

static bool get_pending(struct IRQState* state, uint num) {
    uint8_t mask = 1u << num;
    uint32_t save = save_and_disable_interrupts();
    uint8_t pending = state->pending;
    state->pending &= ~mask;
    restore_interrupts(save);
    return (pending & mask) != 0;
}

static int mod_wait_user_irq_1(lua_State* ls);
static int mod_wait_user_irq_2(lua_State* ls, int status, lua_KContext ctx);

static int mod_wait_user_irq(lua_State* ls) {
    uint num = check_user_irq(ls, 1) - FIRST_USER_IRQ;
    struct IRQState* state = &uirq_state[get_core_num()];
    if (get_pending(state, num)) return 0;
    mlua_event_watch(ls, state->events[num]);
    return mod_wait_user_irq_1(ls);
}

static int mod_wait_user_irq_1(lua_State* ls) {
    return mlua_event_suspend(ls, &mod_wait_user_irq_2);
}

static int mod_wait_user_irq_2(lua_State* ls, int status, lua_KContext ctx) {
    uint num = check_user_irq(ls, 1) - FIRST_USER_IRQ;
    struct IRQState* state = &uirq_state[get_core_num()];
    if (!get_pending(state, num)) return mod_wait_user_irq_1(ls);
    mlua_event_unwatch(ls, state->events[num]);
    return 0;
}

static int mod_clear(lua_State* ls) {
    uint irq = check_user_irq(ls, 1);
    struct IRQState* state = &uirq_state[get_core_num()];
    uint num = irq - FIRST_USER_IRQ;
    uint32_t save = save_and_disable_interrupts();
    irq_clear(irq);
    state->pending &= ~(1u << num);
    mlua_event_set(state->events[num], false);
    restore_interrupts(save);
    return 0;
}

static int mod_set_enabled(lua_State* ls) {
    uint irq = check_user_irq(ls, 1);
    bool enabled = mlua_to_cbool(ls, 2);
    // Clear pending state before enabling. irq_set_enabled does the same.
    if (enabled) mod_clear(ls);
    irq_set_enabled(irq, enabled);
    return 0;
}

MLUA_FUNC_0_2(mod_, irq_, set_priority, check_irq, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, irq_, get_priority, lua_pushinteger, check_irq)
MLUA_FUNC_1_1(mod_, irq_, is_enabled, lua_pushboolean, check_irq)
MLUA_FUNC_0_2(mod_, irq_, set_mask_enabled, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, irq_, has_shared_handler, lua_pushboolean, check_irq)
MLUA_FUNC_0_1(mod_, irq_, set_pending, check_user_irq)
MLUA_FUNC_0_1(mod_,, user_irq_claim, check_user_irq)
MLUA_FUNC_1_1(mod_,, user_irq_claim_unused, lua_pushinteger, mlua_to_cbool)
MLUA_FUNC_0_1(mod_,, user_irq_unclaim, check_user_irq)

static mlua_reg const module_regs[] = {
#define MLUA_SYM(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    MLUA_SYM(TIMER_IRQ_0),
    MLUA_SYM(TIMER_IRQ_1),
    MLUA_SYM(TIMER_IRQ_2),
    MLUA_SYM(TIMER_IRQ_3),
    MLUA_SYM(PWM_IRQ_WRAP),
    MLUA_SYM(USBCTRL_IRQ),
    MLUA_SYM(XIP_IRQ),
    MLUA_SYM(PIO0_IRQ_0),
    MLUA_SYM(PIO0_IRQ_1),
    MLUA_SYM(PIO1_IRQ_0),
    MLUA_SYM(PIO1_IRQ_1),
    MLUA_SYM(DMA_IRQ_0),
    MLUA_SYM(DMA_IRQ_1),
    MLUA_SYM(IO_IRQ_BANK0),
    MLUA_SYM(IO_IRQ_QSPI),
    MLUA_SYM(SIO_IRQ_PROC0),
    MLUA_SYM(SIO_IRQ_PROC1),
    MLUA_SYM(CLOCKS_IRQ),
    MLUA_SYM(SPI0_IRQ),
    MLUA_SYM(SPI1_IRQ),
    MLUA_SYM(UART0_IRQ),
    MLUA_SYM(UART1_IRQ),
    MLUA_SYM(ADC_IRQ_FIFO),
    MLUA_SYM(I2C0_IRQ),
    MLUA_SYM(I2C1_IRQ),
    MLUA_SYM(RTC_IRQ),
    MLUA_SYM(FIRST_USER_IRQ),
    MLUA_SYM(NUM_USER_IRQS),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(integer, n, PICO_ ## n)
    MLUA_SYM(DEFAULT_IRQ_PRIORITY),
    MLUA_SYM(LOWEST_IRQ_PRIORITY),
    MLUA_SYM(HIGHEST_IRQ_PRIORITY),
    MLUA_SYM(SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY),
    MLUA_SYM(SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY),
    MLUA_SYM(SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY),
#undef MLUA_SYM
#define MLUA_SYM(n) MLUA_REG(function, n, mod_ ## n)
    MLUA_SYM(set_priority),
    MLUA_SYM(get_priority),
    MLUA_SYM(set_enabled),
    MLUA_SYM(is_enabled),
    MLUA_SYM(set_mask_enabled),
    // irq_set_exclusive_handler: See enable_user_irq, wait_user_irq
    // irq_get_exclusive_handler: not useful in Lua
    // irq_add_shared_handler: See enable_user_irq, wait_user_irq
    // irq_remove_handler: See enable_user_irq, wait_user_irq
    MLUA_SYM(has_shared_handler),
    // irq_get_vtable_handler: not useful in Lua
    MLUA_SYM(clear),
    MLUA_SYM(set_pending),
    MLUA_SYM(user_irq_claim),
    MLUA_SYM(user_irq_claim_unused),
    MLUA_SYM(user_irq_unclaim),
    MLUA_SYM(enable_user_irq),
    MLUA_SYM(wait_user_irq),
#undef MLUA_SYM
    {NULL},
};

int luaopen_hardware_irq(lua_State* ls) {
    mlua_require(ls, "mlua.event", false);

    // Initialize internal state.
    struct IRQState* state = &uirq_state[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    for (uint i = 0; i < NUM_USER_IRQS; ++i) state->events[i] = -1;
    state->pending = 0;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
