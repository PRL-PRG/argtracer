#include "util.h"

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

