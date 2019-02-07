#ifndef DEBUG_H_
#define DEBUG_H_

#ifdef DEBUG
#define DEBUG_TEST 1
#else
#define DEBUG_TEST 0
#endif


#include <string.h>

// Just file name, no path
#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/**
 * Debug printf macro
 * 
 * C99 required.
 * For dprintf("Hello"), which compiler returns an error, use dprintf("%s\n", "Hello").
 */
#define dprintf(fmt, ...) \
        do { if (DEBUG_TEST) fprintf(stderr, "%s:%d:%s(): " fmt, __FILENAME__, \
                                __LINE__, __func__, __VA_ARGS__); } while (0)



#endif // DEBUG_H_
