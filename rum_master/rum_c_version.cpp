//extern "C" void rum_init(Location slaves[], size_t num_slaves);

#include "rum.h"					// the C++ headers

extern "C" {
#include "rum_c_version.h"			// the C headers

//------------------------------------------------------------------------------
void rum_c_init(const char* machines_csv_file_path){
	rum_init(machines_csv_file_path);
}
//------------------------------------------------------------------------------
void* rum_c_alloc(int size){
	return rum_alloc(size);
}
//------------------------------------------------------------------------------
void* rum_c_alloc_location(int size, Location l){
	return rum_alloc(size, l);
}
//------------------------------------------------------------------------------
void* rum_c_alloc_hash(int size, HASH_TYPE hash){
	return rum_alloc(size, hash);
}
//------------------------------------------------------------------------------
void* rum_c_alloc_hash_location(int size, HASH_TYPE hash, Location l){
	return rum_alloc(size, hash, l);
}
//------------------------------------------------------------------------------
void* rum_c_calloc (int num, int size){
	return rum_calloc(num, size);
}
//------------------------------------------------------------------------------
void* rum_c_realloc (void* ptr, int size){
	return rum_realloc(ptr, size);
}
//------------------------------------------------------------------------------
void rum_c_free(void* ptr){
	rum_free(ptr);
}
//------------------------------------------------------------------------------
void rum_c_stats(void){
	rum_stats();
}
//------------------------------------------------------------------------------
void* rum_c_get(HASH_TYPE hash, Location l){
	return rum_get(hash, l);
}
//------------------------------------------------------------------------------
void rum_c_end(void){
	rum_end();
}
//------------------------------------------------------------------------------
} /*extern C*/
