#include <string.h>

#include "hardware/irq.h"
#include "hardware/sync.h"
#include "pico/platform.h"

#include "lua.h"
#include "lauxlib.h"
#include "mlua/signal.h"
#include "mlua/util.h"

static SigNum irq_signals[NUM_CORES][NUM_IRQS];

void mlua_irq_set_handler(lua_State* ls, uint irq, void (*irq_handler)(void),
                          lua_CFunction handler_wrapper, int handler) {
    bool has_handler = mlua_signal_wrap_handler(ls, handler_wrapper, handler);
    SigNum* sig = &irq_signals[get_core_num()][irq];
    if (*sig < 0) {  // No handler is currently set
        if (!has_handler) return;
        mlua_signal_claim(ls, sig, handler);
        lua_Integer order_priority = luaL_optinteger(ls, handler + 1, -1);
        if (order_priority < 0) {
            irq_set_exclusive_handler(irq, irq_handler);
        } else {
            irq_add_shared_handler(irq, irq_handler, order_priority);
        }
    } else if (has_handler) {  // An existing handler is being updated
        mlua_signal_set_handler(ls, *sig, handler);
    } else {  // An existing handler is being unset
        irq_remove_handler(irq, irq_handler);
        mlua_signal_unclaim(ls, sig);
    }
}

void __time_critical_func(mlua_irq_set_signal)(uint irq, bool pending) {
    mlua_signal_set(irq_signals[get_core_num()][irq], pending);
}

static void __time_critical_func(handle_irq)(void) {
    uint irq = __get_current_exception() - VTABLE_FIRST_IRQ;
    SigNum sig = irq_signals[get_core_num()][irq];
    switch (irq) {
    default:
        irq_clear(irq);
        break;
    }
    mlua_signal_set(sig, true);
}

static lua_Integer check_irq(lua_State* ls, int index) {
    lua_Integer irq = luaL_checkinteger(ls, index);
    luaL_argcheck(ls, 0 <= irq && irq < (lua_Integer)NUM_IRQS, index,
                  "unsupported IRQ number");
    return irq;
}

static int mod_set_handler(lua_State* ls) {
    mlua_irq_set_handler(ls, check_irq(ls, 1), &handle_irq, NULL, 2);
    return 0;
}

static int mod_remove_handler(lua_State* ls) {
    lua_pushnil(ls);
    mlua_irq_set_handler(ls, check_irq(ls, 1), &handle_irq, NULL, -1);
    return 0;
}

static int mod_clear(lua_State* ls) {
    lua_Integer irq = check_irq(ls, 1);
    mlua_irq_set_signal(irq, false);
    irq_clear(irq);
    return 0;
}

MLUA_FUNC_0_2(mod_, irq_, set_priority, luaL_checkinteger, luaL_checkinteger)
MLUA_FUNC_1_1(mod_, irq_, get_priority, lua_pushinteger, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, irq_, set_enabled, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, irq_, is_enabled, lua_pushboolean, luaL_checkinteger)
MLUA_FUNC_0_2(mod_, irq_, set_mask_enabled, luaL_checkinteger, mlua_to_cbool)
MLUA_FUNC_1_1(mod_, irq_, has_shared_handler, lua_pushboolean,
              luaL_checkinteger)
MLUA_FUNC_0_1(mod_, irq_, set_pending, luaL_checkinteger)
MLUA_FUNC_0_1(mod_,, user_irq_claim, luaL_checkinteger)
MLUA_FUNC_1_1(mod_,, user_irq_claim_unused, lua_pushinteger, mlua_to_cbool)
MLUA_FUNC_0_1(mod_,, user_irq_unclaim, luaL_checkinteger)

static mlua_reg const module_regs[] = {
#define X(n) MLUA_REG(function, n, mod_ ## n)
    X(set_priority),
    X(get_priority),
    X(set_enabled),
    X(is_enabled),
    X(set_mask_enabled),
    X(set_handler),
    // irq_set_exclusive_handler: Implemented by set_handler
    // irq_get_exclusive_handler: not useful in Lua
    // irq_add_shared_handler: Implemented by set_handler
    X(remove_handler),
    X(has_shared_handler),
    // irq_get_vtable_handler: not useful in Lua
    X(clear),
    X(set_pending),
    X(user_irq_claim),
    X(user_irq_claim_unused),
    X(user_irq_unclaim),
#undef X
#define X(n) {.name=#n, .push=mlua_reg_push_integer, .integer=n}
    X(TIMER_IRQ_0),
    X(TIMER_IRQ_1),
    X(TIMER_IRQ_2),
    X(TIMER_IRQ_3),
    X(PWM_IRQ_WRAP),
    X(USBCTRL_IRQ),
    X(XIP_IRQ),
    X(PIO0_IRQ_0),
    X(PIO0_IRQ_1),
    X(PIO1_IRQ_0),
    X(PIO1_IRQ_1),
    X(DMA_IRQ_0),
    X(DMA_IRQ_1),
    X(IO_IRQ_BANK0),
    X(IO_IRQ_QSPI),
    X(SIO_IRQ_PROC0),
    X(SIO_IRQ_PROC1),
    X(CLOCKS_IRQ),
    X(SPI0_IRQ),
    X(SPI1_IRQ),
    X(UART0_IRQ),
    X(UART1_IRQ),
    X(ADC_IRQ_FIFO),
    X(I2C0_IRQ),
    X(I2C1_IRQ),
    X(RTC_IRQ),
    X(FIRST_USER_IRQ),
    X(NUM_USER_IRQS),
#undef X
#define X(n) MLUA_REG(integer, n, PICO_ ## n)
    X(DEFAULT_IRQ_PRIORITY),
    X(LOWEST_IRQ_PRIORITY),
    X(HIGHEST_IRQ_PRIORITY),
    X(SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY),
    X(SHARED_IRQ_HANDLER_HIGHEST_ORDER_PRIORITY),
    X(SHARED_IRQ_HANDLER_LOWEST_ORDER_PRIORITY),
#undef X
    {NULL},
};

int luaopen_hardware_irq(lua_State* ls) {
    mlua_require(ls, "mlua.signal", false);

    // Initialize internal state.
    SigNum* sigs = irq_signals[get_core_num()];
    uint32_t save = save_and_disable_interrupts();
    for (int i = 0; i < (int)NUM_IRQS; ++i) sigs[i] = -1;
    restore_interrupts(save);

    // Create the module.
    mlua_newlib(ls, module_regs, 0, 0);
    return 1;
}
