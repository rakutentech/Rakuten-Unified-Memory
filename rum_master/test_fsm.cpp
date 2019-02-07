#include "fsm.h"
#include "util.h"

#include <cstdlib>

int main(int argc, char*argv[]){
	size_t size = 1 * 1024 * 1024 * 1024;

	char *p = (char*)malloc(size);

	mem_init(p, p + size);
        
        
        size_t lim = 2;
        size_t alloc_size;
        char* ptr;
        for(int i = 0 ; i < lim ; i++){
            alloc_size = rand();
            if((ptr =(char*) mem_base_alloc(alloc_size)) == NULL){
                PRINTF("FAIL : %ld\n", alloc_size);
            }
            else{
                PRINTF("SUCC : %ld\n", alloc_size);
            }
            mem_base_free(ptr);
        }
        
}