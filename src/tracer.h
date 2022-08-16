#ifndef TRACER_H
#define TRACER_H

#include <Rinternals.h>

extern "C" {
SEXP trace_code(SEXP, SEXP, SEXP, SEXP);
}

#endif /* TRACER_H */
