#ifndef DPDK_H_
#define DPDK_H_

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>

#include "hash_type.h"
#include "util.h"
#include "Location.h"

typedef uint64_t Addr;


int
dpdk_main(int argc, char *argv[]);

int
dpdk_main(size_t data_pool_bits, size_t page_size_bits);

int
dpdk_shell(int argc, char *argv[]);

int
dpdk_end(void);

void
dpdkInitHostMemory(char *host_memory);

uint64_t
dpdkGetAsync(uint64_t key, uint8_t *ret_buffer);

void
dpdkGet(uint64_t key, uint8_t *ret_buffer);

void
dpdkBulkGet(uint64_t *keys, uint8_t **ret_buffers, size_t size);

void
dpdkWrite(uint64_t page_base);

uint64_t
dpdkSetAsync(uint64_t key, uint8_t *data);

uint64_t
dpdkSetAsyncWait(uint64_t key, uint8_t *data);

void
dpdk_print_port_stats_all(void);

uint64_t
dpdk_stats_get_tx_total(void);

uint64_t
dpdk_stats_get_rx_total(void);

void
dpdk_time_points(const std::string &prefix);

void
dpdk_time_sizes(const std::string &prefix);

void
dpdk_stats(const std::string &prefix);

// uint64_t is an address
uint64_t 	
dpdk_alloc(Location l, size_t size, unsigned char* uuid);

void 	
dpdk_dealloc(Location l, Addr ptr, unsigned char* uuid);

uint64_t 	
dpdk_alloc(Location l, size_t size, HASH_TYPE hash);

void 	
dpdk_dealloc(Location l, HASH_TYPE hash);

void dpdk_write_hash(
    Location    l, 
    HASH_TYPE   hash, 
    size_t      offset, 
    Addr        local_addr,
    size_t      size
);

void dpdk_write(
    Location    l, 
    Addr        remote_ptr, 
    size_t      offset, 
    Addr     	local_ptr, 
    size_t      size
);

void dpdk_read(
    Location    l, 
    HASH_TYPE   hash, 
    Addr        local_addr,
    size_t      offset, 
    size_t      size
);

void dpdk_read(
    Location            l, 
    Addr 		remote_ptr,
    Addr 		local_addr,
    size_t 		offset, 
    size_t 		size
);

void
dpdk_get_hash(
	Location   l, 
	HASH_TYPE  hash, 
	size_t 	   *alloc_size, 
	char	   *remote_ptr
);

#endif
