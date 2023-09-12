#include "mlua/module.h"

#include "mlua/util.h"

MLUA_SYMBOLS(module_syms) = {
@SYMBOLS@
};

MLUA_OPEN_MODULE(@MOD@) {
    mlua_new_module(ls, 0, module_syms);
    return 1;
}
