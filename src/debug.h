#ifndef DEBUG_H
#define DEBUG_H

#include <Rinternals.h>

#define Debug(...) Rprintf("*** " __VA_ARGS__);

#ifndef DEBUG
#undef Debug
#define Debug(...)                                                             \
    do {                                                                       \
    } while (0)
#endif

#endif // DEBUG_H
