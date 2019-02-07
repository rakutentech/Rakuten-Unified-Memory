#ifndef __FSM_H__
#define __FSM_H__

#include <cstdint>

void 	mem_init(void* start, void* end);
void* 	mem_base_alloc(uint64_t nbytes);
void 	mem_base_free(void* ap);

#endif /*__FSM_H__*/