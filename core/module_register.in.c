#include "mlua/module.h"

#cmakedefine SYMBOLS 1

#ifdef SYMBOLS

@INCLUDE@
#include "mlua/util.h"

MLUA_SYMBOLS(module_syms) = {
#include "@SYMBOLS@"
};

MLUA_OPEN_MODULE(@MOD@) {
    mlua_new_table(ls, 0, module_syms);  // Intentionally non-strict
    return 1;
}

#else

MLUA_REGISTER_MODULE(@MOD@, luaopen_@SYM@);

#endif
