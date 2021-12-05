#define USE_RINTERNALS

#include <Rinternals.h>

bool is_freesxp(SEXP x) { return ((SEXP)x)->sxpinfo.type == FREESXP; }
