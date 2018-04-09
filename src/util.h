#ifndef __UTIL_H__
#define __UTIL_H__

char *aprintf(char **s, const char *fmt, ...);

#if defined(USE_DEBUG) && defined(DEBUG_IP)
#include <debugnet.h>
#else
#define debugNetInit(...)
#define debugNetPrintf(...)
#define debugNetFinish(...)
#define DEBUG 3
#define INFO 1
#endif

#endif
