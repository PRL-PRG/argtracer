#include "util.h"
#include "debug.h"

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

void populate_namespace_map(SEXP env,
                            std::unordered_map<SEXP, FunOrigin>& map) {
    auto pkg_name_opt = env_get_name(env);
    if (!pkg_name_opt) {
        return;
    }

    auto pkg_name = pkg_name_opt.value();
    auto bindings = PROTECT(R_lsInternal3(env, TRUE, FALSE));

    Debug("Populating %s\n", pkg_name.c_str());
    for (int i = 0; i < Rf_length(bindings); i++) {
        auto binding = STRING_ELT(bindings, i);
        auto fun = Rf_findVarInFrame3(env, Rf_installChar(binding), TRUE);

        if (fun == R_UnboundValue) {
            continue;
        }

        if (TYPEOF(fun) == PROMSXP && PRVALUE(fun) != R_UnboundValue) {
            fun = PRVALUE(fun);
        }

        if (TYPEOF(fun) != CLOSXP) {
            continue;
        }

        std::string fun_name = CHAR(binding);
        auto fun_origin = std::make_pair(pkg_name, fun_name);
        map.insert(std::make_pair(fun, fun_origin));
    }

    UNPROTECT(1);
}
