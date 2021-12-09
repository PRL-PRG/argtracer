#ifndef UTIL_H
#define UTIL_H

#include <Rinternals.h>
#include <optional>
#include <string>
#include <unordered_map>

std::optional<SEXP> arg_val(SEXP promise);
SEXP get_or_load_binding(SEXP env, SEXP binding);
std::optional<std::string> env_get_name(SEXP);
SEXP env2list(SEXP env);

#endif // UTIL_H
