#include "rum.h"
#include "dpdk.h"		// for location

#include <stdio.h>
#include <string.h>

#include <vector>

int main(int argc, const char * argv[]){

	int alloc_size	= 4096;
	int offset 		= 0; //4100;
	int num_ptrs 	= 4;

	std::vector<Location> slaves = {"thing1", "thing2"};

	printf("Driver: init mem\n");
	rum_init(slaves);

	printf("Driver: alloc memory %d\n", alloc_size);
	char* ptrs[num_ptrs];

	for(int i  = 0 ; i < num_ptrs ; i++){
		if((ptrs[i] = rum_alloc(alloc_size)) == NULL){
			printf("Driver: could not alloc memory of size %d\n", alloc_size);
		}	
		printf("Driver: allocted %d bytes @ 0x%lx\n", alloc_size, ptrs[i]);
	}
	
	printf("Driver: Got memory, now play\n");

	for(int j = 0 ; j < 4 ; j++){
		for(int i  = 0 ; i < num_ptrs ; i++){
			int start_val = rand(), end_val = 0;
			

			memcpy((ptrs[i] + offset), &start_val, sizeof(int));
			memcpy(&end_val, (ptrs[i] + offset), sizeof(int));

			printf("write %d || ", start_val);
			printf("read  %d\n",  end_val);
		}
	}
	printf("Driver: free mem\n");
	for(int i  = 0 ; i < num_ptrs ; i++){
		rum_free(ptrs[i]);
	}
	printf("Driver: done\n");
}