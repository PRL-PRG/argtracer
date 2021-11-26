#ifndef UTIL_H
#define UTIL_H

#include <R/Rinternals.h>
#include <optional>
#include <string>

std::optional<std::string> env_get_name(SEXP);

#endif // UTIL_H
