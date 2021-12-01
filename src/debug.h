#ifndef DEBUG_H
#define DEBUG_H

#include <Rinternals.h>

#define Debug(...) Rprintf("*** " __VA_ARGS__);

#ifdef NDEBUG
#undef Debug
#define Debug(...)                                                             \
    do {                                                                       \
    } while (0)
#endif // NDEBUG

#endif // DEBUG_H
