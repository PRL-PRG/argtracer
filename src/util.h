#ifndef UTIL_H
#define UTIL_H

#include <Rinternals.h>
#include <optional>
#include <string>
#include <unordered_map>

typedef std::pair<std::string, std::string> FunOrigin;

std::optional<std::string> env_get_name(SEXP);
void populate_namespace_map(SEXP env, std::unordered_map<SEXP, FunOrigin>& map);

#endif // UTIL_H
