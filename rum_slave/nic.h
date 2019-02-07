/**
 * NIC related routines
 * 
 * For documentation see the matching source file.
 */

#ifndef NIC_H_
#define NIC_H_

#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#include <rte_pdump.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>


#ifdef __cplusplus
extern "C" {
#endif


void
dpdk_check_link_status(uint8_t port_num, uint32_t port_mask);

void
dpdk_eth_stats_print(size_t portid);


#ifdef __cplusplus
}
#endif

#endif // NIC_H_