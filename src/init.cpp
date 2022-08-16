#include <R_ext/Rdynload.h>
#include <Rinternals.h>
#include <stdlib.h>

#include "tracer.h"

extern "C" {

static const R_CallMethodDef callMethods[] = {
    {"trace_code", (DL_FUNC)&trace_code, 4}, {NULL, NULL, 0}};

void R_init_argtracer(DllInfo* dll) {
    R_registerRoutines(dll, NULL, callMethods, NULL, NULL);
    R_useDynamicSymbols(dll, FALSE);
}
}
