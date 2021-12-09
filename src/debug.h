#ifndef DEBUG_H
#define DEBUG_H

#include <Rinternals.h>
#include <cstdio>

#define DEBUG(...) printf("debug: " __VA_ARGS__);
#define TRACE(...) printf("trace: " __VA_ARGS__);

#ifndef LOG_DEBUG
#undef DEBUG
#define DEBUG(...)                                                             \
    do {                                                                       \
    } while (0)
#endif // NDEBUG

#ifndef LOG_TRACE
#undef TRACE
#define TRACE(...)                                                             \
    do {                                                                       \
    } while (0)
#endif

#endif // DEBUG_H
