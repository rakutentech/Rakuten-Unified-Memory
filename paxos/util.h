#ifndef __UTIL_H__
#define __UTIL_H__

#include <string>		// this is for the current definitions of the following

#include <memory>       // for the shared pointers

// until we decide what a value is
typedef std::string IP_ADDR;

#define DEBUG 1

#if DEBUG
#define PRINTF(...) {printf("\033[1;34mDEBUG:\033[0m "); printf(__VA_ARGS__);}
#define ERROR(...)	{printf("\033[0;31mERROR:\033[0m "); printf(__VA_ARGS__);}
#define TODO(...)	{printf("\033[0;35mTODO:\033[0m "); printf(__VA_ARGS__);}
//static unsigned int debug_counter = 0;
//#define DEBUG_COUNT() 	{PRINTF("cnt - %d\n", debug_counter++);}

#else
#define PRINTF(...)
#define ERROR(...)
#define TODO(...)
#define DEBUG_COUNT()
#endif



//size_t value_marshal(VALUE v, char* buf, size_t len, bool encode);
//VALUE value_demarshal(char* buf, size_t len, size_t* ret_pos);
#endif /*__UTIL_H__*/