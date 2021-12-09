#include "util.h"
#include "debug.h"

SEXP get_or_load_binding(SEXP env, SEXP binding) {
    auto val = Rf_findVarInFrame3(env, binding, TRUE);

    if (TYPEOF(val) == PROMSXP) {
        if (PRVALUE(val) != R_UnboundValue) {
            val = PRVALUE(val);
        } else {
            // if the binding has not yet been loaded, we force it now
            val = Rf_eval(val, env);
        }
    }

    return val;
}

std::optional<std::string> env_get_name(SEXP env) {
    if (R_IsPackageEnv(env) || R_IsNamespaceEnv(env)) {
        std::string name;
        if (R_IsPackageEnv(env)) {
            name = CHAR(STRING_ELT(R_PackageEnvName(env), 0));
        } else {
            name = CHAR(STRING_ELT(R_NamespaceEnvSpec(env), 0));
        }
        return name;
    } else {
        return {};
    }
}

SEXP env2list(SEXP env) {
    return Rf_eval(Rf_lang4(Rf_install("as.list.environment"), env,
                            Rf_ScalarLogical(TRUE), Rf_ScalarLogical(FALSE)),
                   R_GlobalEnv);
}
