#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define debug(...) printf(__VA_ARGS__)
#else
#define debug(...)
#endif

#endif /* DEBUG_H */
