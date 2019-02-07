#ifndef __RUM_H__
#define __RUM_H__

#include <stddef.h>		// for size_t
#include <stdint.h>		// for uint16_t, etc

#include "hash_type.h"          // for the HASH_TYPE
#include "Location.h"           // For the location

//------------------------------------------------------------------------------
/**
 * Initialize HPC memory.
 */
//void rum_init();
//------------------------------------------------------------------------------
/**
 * Initialize HPC memory.
 * \param servers vector of servers to ask for memory space
 */
//void rum_init(std::vector<Location> slaves);
//------------------------------------------------------------------------------
/**
 * Initialize HPC memory. C version
 * \param machines_csv_file_path	Path to the csv containing the servers
 *									where the path includes "machines.csv"
 */
void rum_init(const char* machines_csv_file_path);
//------------------------------------------------------------------------------
/**
 * Alocate memory from RUM
 * 
 * \param size The size in bytes to be allocated
 * \return Pointer to the alloced memory or NULL on failure
 */
void* rum_alloc(int size);
//------------------------------------------------------------------------------
/**
 * Alocate memory from RUM at a specific location
 * 
 * \param size The size in bytes to be allocated
 * \param location The location where the ram should be allocated from
 * \return Pointer to the alloced memory or NULL on failure
 */
void* rum_alloc(int size, Location l);
//------------------------------------------------------------------------------
/**
 * Alocate memory from RUM, and identify with the supplied hash
 * 
 * \param size The size in bytes to be allocated
 * \param hash 32 bytes + /0 hash string
 * \return Pointer to the alloced memory or NULL on failure
 */
//char* rum_alloc(size_t size, std::string hash);
void* rum_alloc(int size, HASH_TYPE hash);
//------------------------------------------------------------------------------
/**
 * Alocate memory from RUM, and identify with the supplied hash
 * 
 * \param size The size in bytes to be allocated
 * \param hash 32 bytes + /0 hash string
 * \param location The location where the ram should be allocated from
 * \return Pointer to the alloced memory or NULL on failure
 */
//char* rum_alloc(size_t size, std::string hash, Location l);
void* rum_alloc(int size, HASH_TYPE hash, Location l);
//------------------------------------------------------------------------------
/**
 * Allocates a block of memory for an array of num elements, each of them size
 * bytes long, and initializes all its bits to zero.
 *
 * \param num  the number of elements to allocate
 * \param size the size of each element
 * \return A pointer to zero'd memory of size 'num'*'size' or NULL on failure
 */
void* rum_calloc (int num, int size);
//------------------------------------------------------------------------------
/**
 * Allocate a pointer of size 'size' and copy the conents into the returned 
 * pointer. The lesser of ptr's size or 'size' will be copied to the new pointer 
 * 
 * \param  ptr  Pointer to the existing memory
 * \param  size The amount of memory to allocate for the new pointer
 * \return NULL On failure, or a new pointer to memory of length 'size' with 
 *              as much state from ptr copied as possible
 */
void* rum_realloc (void* ptr, int size);
//------------------------------------------------------------------------------
/**
 * Free memory back to RUM
 * \param ptr The pointer to the memory to be free'd
 */
void rum_free(void* ptr);
//------------------------------------------------------------------------------
/**
 * Free memory back to RUM
 * \param ptr The pointer to the memory to be free'd
 * \param h hash of the pointer
 */
void rum_free(void* ptr, HASH_TYPE h);
//------------------------------------------------------------------------------
/**
 * Free memory back to RUM
 * \param l The location of the pointer to be free'd
 * \param h hash of the pointer
 */
void rum_free(Location l, HASH_TYPE h);
//------------------------------------------------------------------------------
/**
 * Print Stats to stdout
 */
void rum_stats(void);
//------------------------------------------------------------------------------
/**
 * This will see if the store has a memory location associated with the hash
 * 
 * \param hash The hash of the memory to look for
 * \return Pointer to the alloced memory if found, or NULL if not found
 */
//char* rum_get(std::string hash);

//char* rum_get(std::string hash, Location l);
void* rum_get(HASH_TYPE hash, Location l);
//------------------------------------------------------------------------------
/**
 * Ends the RUM environment. Provided to be called incase an application crashes.
 * This should be called in the clearup routine
 */
void rum_end(void);
//------------------------------------------------------------------------------
/**
 * Clear out the local cache, flushing any locally stored data to the slaves
 */
void rum_flush_cache(void);
#endif  /*__RUM_H__*/