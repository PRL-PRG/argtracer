#ifndef TRACER_H
#define TRACER_H

#include <R/Rinternals.h>

extern "C" {
SEXP trace_code(SEXP, SEXP);
}

#endif /* TRACER_H */
