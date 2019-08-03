#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG
#define debug(x...) printf(x)
#else
#define debug(x...)
#endif

#endif /* DEBUG_H */
