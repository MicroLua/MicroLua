#include "mlua/module.h"

@INCLUDE@
#include "mlua/util.h"

MLUA_SYMBOLS(module_syms) = {
#include "@SYMBOLS@"
};

MLUA_OPEN_MODULE(@MOD@) {
    mlua_new_table(ls, 0, module_syms);  // Intentionally non-strict
    return 1;
}
