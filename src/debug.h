#ifndef DEBUG_H
#define DEBUG_H

#include <Rinternals.h>
#include <cstdio>

#define Debug(...) printf("*** " __VA_ARGS__);
#define Trace(...) printf(__VA_ARGS__);

#ifdef NDEBUG
#undef Debug
#define Debug(...)                                                             \
    do {                                                                       \
    } while (0)
#endif // NDEBUG

#ifndef TRACE
#undef Trace
#define Trace(...)                                                             \
    do {                                                                       \
    } while (0)
#endif

#endif // DEBUG_H
