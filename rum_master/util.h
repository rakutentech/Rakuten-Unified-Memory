#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>		// this is for the current definitions of the following

#define DEBUG 1

#if DEBUG
#define PRINTF(...) {printf("\033[1;34mDEBUG:\033[0m "); printf(__VA_ARGS__);}
#define ERROR(...)	{printf("\033[0;31mERROR:\033[0m "); printf(__VA_ARGS__);}
#define TODO(...)	{printf("\033[0;35mTODO:\033[0m "); printf(__VA_ARGS__);}
#else
#define PRINTF(...)
#define ERROR(...)
#define TODO(...)
#endif

#endif /*__UTIL_H__*/