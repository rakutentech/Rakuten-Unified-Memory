#ifndef __DPDK_H__
#define __DPDK_H__
/**
 * This is basically an interface between RUM and the actual DPDK libraries.
 * -> hence it will depend on the DPDK libraries.
 */
#include <string>

using namespace std;

typedef uint64_t Addr;			// this is already defined elsewhere
typedef string Location;

typedef enum dpdk_query_type {
	available_space = 0,
	allocate,
	deallocate,
} DPDK_QUERY_TYPE;

int 	dpdk_read(Location l, 	Addr data, size_t size);
int		dpdk_write(Location l, 	Addr data, size_t size);
int		dpdk_query(Location l, DPDK_QUERY_TYPE t);
Addr 	dpdk_alloc(Location l, size_t size);

/*----------------------------------------------------------------------------*/
// Debug commands

int dpdk_shell(int argc, char *argv[]);
/*----------------------------------------------------------------------------*/
#endif /*__DPDK_H__*/