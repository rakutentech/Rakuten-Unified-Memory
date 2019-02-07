/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2016 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#include <iostream>			// file access
#include <fstream>			// file access

#include <boost/filesystem.hpp>     // for the directory stuff

#include "ipv6.h"
#include "nic.h"
#include "../rum_master/packet.h"
#include "SlavePaxos.h"
#include <map>

#include "../paxos/paxos_instance.h"
#include "../paxos/node.h"
#include "slave_paxos_msg.h"
#include "Heartbeat.h"

//////////////////////////
// for the get_ip_address
#include <sys/types.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
//////////////////////////
//------------------------------------------------------------------------------
#define DEBUG 1

#if DEBUG
#define PRINTF(...) {printf("\033[1;34mDEBUG:\033[0m "); printf(__VA_ARGS__);}
#define ERROR(...)  {printf("\033[0;31mERROR:\033[0m "); printf(__VA_ARGS__);}
#define TODO(...)   {printf("\033[0;35mTODO:\033[0m ");  printf(__VA_ARGS__);}
#else
#define PRINTF(...)
#define ERROR(...)
#define TODO(...)
#endif
//------------------------------------------------------------------------------

static volatile bool force_quit;

/* MAC updating enabled by default */
static int mac_updating = 0; // unused

#define RTE_LOGTYPE_L2FWD RTE_LOGTYPE_USER1

//#define NB_MBUF   8192
#define NB_MBUF   ((1UL<<15)-1)

#define MAX_PKT_BURST 32 //8 //16 //32 //512 //128
#define BURST_TX_DRAIN_US 30 //15 //100 /* TX drain every ~100us */
#define MEMPOOL_CACHE_SIZE 256

/* allow max jumbo frame 9.5 KB */
#define	JUMBO_FRAME_MAX_SIZE	0x2600
#define PREFETCH_OFFSET 3
#define DRAIN_WAIT_IN_US 4
#define TX_RETRY_COUNT 10000

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 128*8
#define RTE_TEST_TX_DESC_DEFAULT 512*8
static uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
static uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* ethernet addresses of ports */
static struct ether_addr l2fwd_ports_eth_addr[RTE_MAX_ETHPORTS];

/* mask of enabled ports */
static uint32_t l2fwd_enabled_port_mask = 0;

/* list of enabled ports */
static uint32_t l2fwd_dst_ports[RTE_MAX_ETHPORTS];

static unsigned int l2fwd_rx_queue_per_lcore = 1;

#define MAX_RX_QUEUE_PER_LCORE 16
#define MAX_TX_QUEUE_PER_PORT 16
struct lcore_queue_conf {
	unsigned n_rx_port;
	unsigned rx_port_list[MAX_RX_QUEUE_PER_LCORE];
} __rte_cache_aligned;
struct lcore_queue_conf lcore_queue_conf[RTE_MAX_LCORE];

static struct rte_eth_dev_tx_buffer *tx_buffer[RTE_MAX_ETHPORTS];

struct rte_mempool * l2fwd_pktmbuf_pool = NULL;

/* Per-port statistics struct */
struct l2fwd_port_statistics {
	uint64_t tx;
	uint64_t rx;
	uint64_t dropped;
	uint64_t rx_type[END_PACKET];
	uint64_t forwarded;
	uint64_t retried;
} __rte_cache_aligned;
struct l2fwd_port_statistics port_statistics[RTE_MAX_ETHPORTS];

#define MAX_TIMER_PERIOD 86400 /* 1 day max */
/* A tsc-based timer responsible for triggering statistics printout */
static uint64_t timer_period = 15; /* default period is 10 seconds */

static size_t l2fwd_data_pool_bits; // 32 = 4GB
static size_t l2fwd_data_pool_size;
static uint8_t *l2fwd_data_pool;
static size_t l2fwd_page_size_bits; // 12 = 4KB
static size_t l2fwd_page_size;
static size_t l2fwd_nb_pages;

typedef uint64_t Addr;
static std::map<std::string, Addr> data_map;	// maps (user) hash -> local ptr
static std::map<Addr, size_t> data_size_map;	// maps pointer to its size
static std::map<std::string, std::vector<Addr>*> uuid_memory_map; // maps uuid to the allocated memory

#define SIZE_T_P(a)		((size_t*)(a))

// some simple accounting stats
size_t total_bytes_allocated 	= 0;
size_t total_bytes_freed 		= 0;

#define TOTAL_BYTES_IN_USE() 	(total_bytes_allocated - total_bytes_freed)

//#define USE_PAXOS

#ifdef USE_PAXOS
// the paxos resilliance variables
static SlavePaxos       *pax_slave                  = NULL;
static paxos_instance   *pax_inst                   = NULL;
static std::string      pax_backup_filename;
static std::thread      pax_daemon_thread;
static std::thread      pax_client_daemon_thread;
#endif

// Heartbeat for flushing ram of dead processes
static Heartbeat *hb;
//------------------------------------------------------------------------------
// function to set some values
inline void dpdk_init_port_conf(struct rte_eth_conf *conf)
{
	memset(conf, 0, sizeof(struct rte_eth_conf));

	conf->rxmode.split_hdr_size = 0;
	conf->rxmode.header_split   = 0; // Header Split disabled
	conf->rxmode.hw_ip_checksum = 0; // IP checksum offload disabled
	conf->rxmode.hw_vlan_filter = 0; // VLAN filtering disabled
		//.max_rx_pkt_len = JUMBO_FRAME_MAX_SIZE;
        //.max_rx_pkt_len = ETHER_MAX_LEN;
    conf->rxmode.max_rx_pkt_len = 8192;
//    conf->rxmode.max_rx_pkt_len = JUMBO_FRAME_MAX_SIZE;
	conf->rxmode.jumbo_frame    = 1; // Jumbo Frame Support enabled
	conf->rxmode.hw_strip_crc   = 1; // CRC stripped by hardware

	conf->txmode.mq_mode = ETH_MQ_TX_NONE;
}
//------------------------------------------------------------------------------
static void l2fwd_init_data(size_t numa_socket){
	l2fwd_data_pool_size = 1UL << l2fwd_data_pool_bits;
	l2fwd_page_size = 1UL << l2fwd_page_size_bits;
	l2fwd_nb_pages = 1UL << (l2fwd_data_pool_bits - l2fwd_page_size_bits);
	printf("l2fwd_data_pool_bits %lu\n", l2fwd_data_pool_bits);
	printf("l2fwd_data_pool_size %lu\n", l2fwd_data_pool_size);
	printf("l2fwd_page_size_bits %lu\n", l2fwd_page_size_bits);
	printf("l2fwd_page_size %lu\n", l2fwd_page_size);
	printf("l2fwd_nb_pages %lu\n", l2fwd_nb_pages);

	//l2fwd_data_pool = rte_zmalloc("DATA_POOL", l2fwd_data_pool_size, 0);
	l2fwd_data_pool = (uint8_t*)rte_zmalloc_socket("DATA_POOL", l2fwd_data_pool_size, 0, numa_socket);
	if (l2fwd_data_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Cannot init data pool\n");
	}
}
//------------------------------------------------------------------------------
/* Print out statistics on packets dropped */
static void print_stats(void){
	uint64_t total_packets_dropped, total_packets_tx, total_packets_rx;
	unsigned portid;
	size_t total_rx_type[END_PACKET];
	size_t total_packets_forwarded;
	size_t total_packets_retried;
	size_t i;

	total_packets_dropped = 0;
	total_packets_tx = 0;
	total_packets_rx = 0;
	for (i = 0; i < END_PACKET; i++)
		total_rx_type[i] = 0;
	total_packets_forwarded = 0;
	total_packets_retried = 0;

	//const char clr[] = { 27, '[', '2', 'J', '\0' };
	//const char topLeft[] = { 27, '[', '1', ';', '1', 'H','\0' };

		/* Clear screen and move to top left */
	//printf("%s%s", clr, topLeft);

	printf("Port statistics ====================================\n");

	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++) {
		/* skip disabled ports */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("\nStatistics for port %u ------------------------------"
			   "\nPackets sent:   %24lu"//PRIu64
			   "\nPackets received:   %20lu"//PRIu64
			   "\nPackets dropped:   %21lu",//PRIu64,
			   portid,
			   port_statistics[portid].tx,
			   port_statistics[portid].rx,
			   port_statistics[portid].dropped);
		printf(
				 "\n     UNKNOWN:    %23lu"
				 "\n        READ:    %23lu"
				 "\n   READ_HASH:    %23lu"
				 "\n       WRITE:    %23lu"
				 "\n  WRITE_HASH:    %23lu"
				 "\n         GET:    %23lu"
			 	 "\n   GET_REPLY:    %23lu"
				 "\n       QUERY:    %23lu"
				 "\n        DATA:    %23lu"
				 "\n      ANSWER:    %23lu"
				 "\n       ALLOC:    %23lu"
				 "\n     DEALLOC:    %23lu"
				 "\n  ALLOC_HASH:    %23lu"
				 "\nDEALLOC_HASH:    %23lu"
			   "\nPackets forwarded:   %19lu"
				 "\nPackets retried:   %21lu",
				 port_statistics[portid].rx_type[UNKNOWN],
				 port_statistics[portid].rx_type[READ],
				 port_statistics[portid].rx_type[READ_HASH],
				 port_statistics[portid].rx_type[WRITE],
				 port_statistics[portid].rx_type[WRITE_HASH],
				 port_statistics[portid].rx_type[GET],
				 port_statistics[portid].rx_type[GET_REPLY],
				 port_statistics[portid].rx_type[QUERY],
				 port_statistics[portid].rx_type[DATA],
				 port_statistics[portid].rx_type[ANSWER],
				 port_statistics[portid].rx_type[ALLOC],
				 port_statistics[portid].rx_type[DEALLOC],
				 port_statistics[portid].rx_type[ALLOC_HASH],
				 port_statistics[portid].rx_type[DEALLOC_HASH],
				 port_statistics[portid].forwarded,
				 port_statistics[portid].retried);

		total_packets_dropped += port_statistics[portid].dropped;
		total_packets_tx += port_statistics[portid].tx;
		total_packets_rx += port_statistics[portid].rx;
		for (i = 0; i < END_PACKET; i++)
			total_rx_type[i] += port_statistics[portid].rx_type[i];
		total_packets_forwarded += port_statistics[portid].forwarded;
		total_packets_retried += port_statistics[portid].retried;
	}
	printf("\nAggregate statistics ==============================="
		   "\nTotal packets sent:   %18lu"//PRIu64
		   "\nTotal packets received:   %14lu"//PRIu64
		   "\nTotal packets dropped:   %15lu",//PRIu64,
		   total_packets_tx,
		   total_packets_rx,
		   total_packets_dropped);
	printf(
			 "\n      UNKNOWN:   %23lu"
			 "\n        READ:    %23lu"
			 "\n   READ_HASH:    %23lu"
			 "\n       WRITE:    %23lu"
			 "\n  WRITE_HASH:    %23lu"
			 "\n         GET:    %23lu"
			 "\n   GET_REPLY:    %23lu"
			 "\n       QUERY:    %23lu"
			 "\n        DATA:    %23lu"
			 "\n      ANSWER:    %23lu"
			 "\n       ALLOC:    %23lu"
			 "\n     DEALLOC:    %23lu"
			 "\n  ALLOC_HASH:    %23lu"
			 "\nDEALLOC_HASH:    %23lu"
			 "\nTotal packets forwarded:   %13lu"
			 "\nTotal packets retried:   %15lu",
 			 total_rx_type[UNKNOWN], 
			 total_rx_type[READ],
			 total_rx_type[READ_HASH],
			 total_rx_type[WRITE],
			 total_rx_type[WRITE_HASH],
			 total_rx_type[GET],
			 total_rx_type[GET_REPLY],
			 total_rx_type[QUERY],
			 total_rx_type[DATA],
			 total_rx_type[ANSWER],
			 total_rx_type[ALLOC],
			 total_rx_type[DEALLOC],
			 total_rx_type[ALLOC_HASH],
			 total_rx_type[DEALLOC_HASH],
			 total_packets_forwarded,
			 total_packets_retried);
	printf("\nTotal bytes allocated %21ld", total_bytes_allocated);
	printf("\nTotal bytes free'd    %21ld", total_bytes_freed);
	printf("\n====================================================\n");
}
//------------------------------------------------------------------------------
static inline void pktmbuf_free_bulk(struct rte_mbuf *mbuf_table[], unsigned n){
	unsigned int i;
	for (i = 0; i < n; i++)
		rte_pktmbuf_free(mbuf_table[i]);
}
//------------------------------------------------------------------------------
/*
 * rte_eth_tx_buffer_set_err_callback()
 */
static void tx_buffer_retransmit_callback(
	struct rte_mbuf **unsent, 
	uint16_t count, 
	void *userdata __rte_unused
	//uint64_t *portid
){
	size_t sent = send_burst_wait(0, unsent, count, 50);
	port_statistics[0].tx += sent;
	port_statistics[0].retried += sent;
	port_statistics[0].dropped = count - sent;
}
//------------------------------------------------------------------------------
#define ETHER_ADDR_LEN_BYTES 6
bool compare_ether_hdr(struct ether_addr first, struct ether_addr second){
	uint8_t *a = first.addr_bytes;
    uint8_t *b = second.addr_bytes;
    
    //printf("== Ethernet ==\n");
    //printf("first  MAC %02X %02X %02X %02X %02X %02X\n", a[0], a[1], a[2], a[3], a[4], a[5]);
    //printf("second MAC %02X %02X %02X %02X %02X %02X\n", b[0], b[1], b[2], b[3], b[4], b[5]);
    
    int i;
    for(i = 0 ; i < ETHER_ADDR_LEN_BYTES ; i++){
    	//printf("[%d] : %02X, %02X\n", i, a[i], b[i]);
    	if(a[i] != b[i]){
    		return 0;
    	}
    }
    return 1;
}
//------------------------------------------------------------------------------
std::string get_ip_address(){
	std::string path 			= "/sys/class/net";
	std::string dev_name;
	std::vector<std::string> devs;
	std::string ipAddress 		= "Unable to get IP Address";
	bool		ip_set 			= false;
    struct ifaddrs *interfaces 	= NULL;
    struct ifaddrs *temp_addr 	= NULL;
    int success 				= 0;

    // get the most likely dev to have the IP address
	for (auto & p : boost::filesystem::directory_iterator(path)){
        dev_name = p.path().leaf().string();
        if(dev_name.compare("lo") != 0){
        	devs.push_back(dev_name);	
        }
    }

    std::sort(devs.begin(), devs.end()); 

    // retrieve the current interfaces - returns 0 on success
    if ((success = getifaddrs(&interfaces)) == 0) {
    	for(std::string dev : devs){
    		temp_addr = interfaces;	
    		while(temp_addr != NULL) {
	            if(temp_addr->ifa_addr->sa_family == AF_INET) {
	                // Check if interface is en0 which is the wifi connection on the iPhone
	                if(strcmp(temp_addr->ifa_name, dev.c_str()) == 0){
	                	std::string tmp = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
	                    //PRINTF("IP ADDRESS:: %s for %s\n", tmp.c_str(), dev.c_str());
	                    if(!ip_set){
	                    	ipAddress = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
	                    	ip_set = true;
	                    }
	                }
	            }
	            temp_addr = temp_addr->ifa_next;
	        }
    	}
        // Loop through linked list of interfaces
        temp_addr = interfaces;
        while(temp_addr != NULL) {
            if(temp_addr->ifa_addr->sa_family == AF_INET) {
                // Check if interface is en0 which is the wifi connection on the iPhone
                if(strcmp(temp_addr->ifa_name, dev_name.c_str()) == 0){
                    ipAddress = inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
                }
            }
            temp_addr = temp_addr->ifa_next;
        }
    }
    // Free memory
    freeifaddrs(interfaces);
    //PRINTF("FINAL:: IP ADDRESS:: %s for %s\n", ipAddress.c_str(), dev_name.c_str());
    return ipAddress;
}
//------------------------------------------------------------------------------
void user_callback_function(std::string hash){
	PRINTF("user_callback_function - %s\n", hash.c_str());
	std::map<std::string, std::vector<Addr>*>::iterator uuid_iter;
	std::map<Addr, size_t>::iterator 					size_map_itter;
	std::vector<Addr>::iterator 						adder_iter;

	if((uuid_iter = uuid_memory_map.find(hash)) != uuid_memory_map.end()){
		// for each allocated address
		for(adder_iter = uuid_iter->second->begin(); adder_iter != uuid_iter->second->end() ; adder_iter++){
			if((size_map_itter = data_size_map.find((Addr)*adder_iter)) != data_size_map.end()){
				
				PRINTF("user_callback_function: free %lx (%ld bytes)\n", (Addr)*adder_iter, size_map_itter->second);
				total_bytes_freed += size_map_itter->second;

				// we don't free things allocated with hash, as that should 
				// be persistent
				free((void*)(Addr)*adder_iter);
				data_size_map.erase(size_map_itter);
			}
			/*
			else{
				// if here, the memory was already free'd, so all good
				PRINTF("user_callback_function: did not find %lx\n", (Addr)*adder_iter);
			}
			*/
		}
	}
	/*
	else{
		PRINTF("user_callback_function: did not find %s\n", hash.c_str());
		int count = 0;
		for(uuid_iter = uuid_memory_map.begin() ; uuid_iter != uuid_memory_map.end() ; uuid_iter++){
			printf("[%d] {%s, [%ld elements]}\n", count++, uuid_iter->first.c_str(), uuid_iter->second->size());
			if(uuid_iter->first.compare(hash) != 0){
				printf("'%s' (%ld bytes) != '%s' (%ld bytes)\n", hash.c_str(), hash.length(), uuid_iter->first.c_str(), uuid_iter->first.length());
			}
		}
	}
	*/
	// now remove the func
	uuid_memory_map.erase(hash);
}
//------------------------------------------------------------------------------
static void l2fwd_simple_forward(struct rte_mbuf *m, unsigned portid){
	if (portid>0) {
		printf("portid %u\n", portid);
		rte_exit(EXIT_FAILURE, "portid>0\n");
	}
	unsigned 						dst_port;
	int 							sent;
	struct rte_eth_dev_tx_buffer *	buffer;
	uint8_t*						data;
	size_t 							size;
	size_t 							offset;
	char* 							tmp;
	int 							count;
	std::map<std::string, Addr>::iterator    	ptr_itter;
	std::string 					hash;
	std::string 					tmp_hash;
	std::string 					uuid;
	std::map<std::string,Addr>::iterator 	ptr_map_itter;
	std::map<Addr, size_t>::iterator 		size_map_itter;
	std::map<std::string, std::vector<Addr>*>::iterator 	uuid_iter;
    //HASH_TYPE                                               ht;

	dst_port = l2fwd_dst_ports[portid];

	struct ether_hdr* recv_e_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);    
#if 0
	printf("ETH_HDR - SRC : %02X %02X %02X %02X %02X %02X\n", recv_e_hdr->s_addr.addr_bytes[0], recv_e_hdr->s_addr.addr_bytes[1], recv_e_hdr->s_addr.addr_bytes[2], recv_e_hdr->s_addr.addr_bytes[3], recv_e_hdr->s_addr.addr_bytes[4], recv_e_hdr->s_addr.addr_bytes[5]);
	printf("ETH_HDR - DST : %02X %02X %02X %02X %02X %02X\n", recv_e_hdr->d_addr.addr_bytes[0], recv_e_hdr->d_addr.addr_bytes[1], recv_e_hdr->d_addr.addr_bytes[2], recv_e_hdr->d_addr.addr_bytes[3], recv_e_hdr->d_addr.addr_bytes[4], recv_e_hdr->d_addr.addr_bytes[5]);
#endif

	if(!compare_ether_hdr(recv_e_hdr->d_addr, l2fwd_ports_eth_addr[portid])){
		//printf("DROP\n");
		rte_pktmbuf_free(m);
		port_statistics[portid].dropped++;
		return;
	}

	//printf_packet(m);
	RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, rum_hdr_t*, ETHER_HDR_LEN);
	switch (rum_hdr->type) {
		case UNKNOWN:
			port_statistics[dst_port].rx_type[UNKNOWN]++;
			rte_pktmbuf_free(m);
			break;
		case READ:
			port_statistics[dst_port].rx_type[READ]++;
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

			// get the data from the packet and push to local data
			offset 		= rum_hdr->offset;
			tmp 		= (char*)rum_hdr->slave_ptr_addr;
			//size 		= rum_hdr->data_len;
			memcpy(&size, &rum_hdr->data_len, sizeof(size_t));

			//printf("READ: offset %ld, size %ld, ptr %lx\n", offset, size, (Addr)tmp);

			// now create a data_packet to return
			create_data_packet(
					m, 
					dst_port,
					recv_e_hdr->s_addr.addr_bytes,
					(uint8_t*)(tmp + offset),
					size, 
					rum_hdr->slave_ptr_addr, 
					rum_hdr->client_ptr_addr, 
					rum_hdr->pak_num );

			// and send
			buffer = tx_buffer[dst_port];
			sent = rte_eth_tx_buffer(dst_port, 0, buffer, m); //reply);
			if (sent) {
				port_statistics[dst_port].tx += sent;
				//rte_delay_us_block(DRAIN_WAIT_IN_US);
			}
			break;	
		case READ_HASH:
			port_statistics[dst_port].rx_type[READ_HASH]++;
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

			// get the data from the packet and push to local data
			tmp_hash.assign((char*) (data), 33);
			offset 		= rum_hdr->offset;
			size 		= rum_hdr->data_len;

			if((ptr_map_itter = data_map.find(tmp_hash)) == data_map.end()){
				ERROR("READ_HASH: could not find hash\n");
				TODO("ERROR PACKET\n");
			}
			else{
				tmp = (char*)ptr_map_itter->second;
				//printf("READ_HASH: offset %ld, size %ld, ptr %lx\n", offset, rum_hdr->data_len, (Addr)tmp);
				//printf("hash [%ld] %s\n", tmp_hash.length(), tmp_hash.c_str());
				//printf("READ_HASH: record [%s, %lx]\n", tmp_hash.c_str(), (Addr)tmp);

				// now create a data_packet to return
				create_data_packet(
					m, //reply,
					dst_port,
					recv_e_hdr->s_addr.addr_bytes,
					(uint8_t*)(tmp + offset),
					size,
					rum_hdr->slave_ptr_addr, 
					rum_hdr->client_ptr_addr, 
					rum_hdr->pak_num );
			}
			

			// and send
			buffer = tx_buffer[dst_port];
			sent = rte_eth_tx_buffer(dst_port, 0, buffer, m); //reply);
			if (sent) {
				port_statistics[dst_port].tx += sent;
				//rte_delay_us_block(DRAIN_WAIT_IN_US);
			}
			break;	
		case WRITE:
			port_statistics[dst_port].rx_type[WRITE]++;
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

			offset 		= rum_hdr->offset;
			tmp 		= (char*)rum_hdr->slave_ptr_addr;
			//size 		= rum_hdr->data_len;
			memcpy(&size, &rum_hdr->data_len, sizeof(size_t));
			
			//printf("WRITE: [%ld bytes] %s\n", size, (char*)(data));
			//printf("WRITE: offset %ld, size %d, ptr %lx\n", offset, size, (Addr)tmp);
			//printf("WRITE: hash [%ld] %s\n", tmp_hash.length(), tmp_hash.c_str());
			//printf("WRITE: record [%s, %lx]\n", tmp_hash.c_str(), (Addr)tmp);
			/*
			count = 0;
			for (std::map<std::string,Addr>::iterator it=data_map.begin(); it!=data_map.end(); ++it){
				printf("WRITE_HASH: MAP[%d] [%s, %lx]\n", count++, it->first.c_str(), (Addr)it->second);
			}
			*/

			//**************************************
			// we assume that we do not get bad data values, so no checking...
			//**************************************

			//printf("%s:%i: WRITE data[0]: 0x%02X\n", __func__, __LINE__, data[0]);
			rte_memcpy((void*)(tmp + offset), data, size);
			rte_pktmbuf_free(m);
#ifdef USE_PAXOS
            // the paxos bit
            pax_slave->write((Addr)tmp, offset, size, (tmp + offset));
#endif
			break;
		case WRITE_HASH:
			port_statistics[dst_port].rx_type[WRITE_HASH]++;

			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

			// get the data from the packet and push to local data
			tmp_hash.assign((char*) (data + rum_hdr->data_len), 33);
			offset 		= rum_hdr->offset;
			//size 		= rum_hdr->data_len;
			memcpy(&size, &rum_hdr->data_len, sizeof(size_t));

			if((ptr_map_itter = data_map.find(tmp_hash)) == data_map.end()){
				ERROR("WRITE_HASH: could not find hash\n");
				//todo: error packet...
			}
			else{
				tmp = (char*)ptr_map_itter->second;
				//printf("WRITE_HASH: offset %ld, size %ld, ptr %lx\n", offset, rum_hdr->data_len, (Addr)tmp);
				//printf("hash [%ld] %s\n", tmp_hash.length(), tmp_hash.c_str());
				//printf("WRITE_HASH: record [%s, %lx]\n", tmp_hash.c_str(), (Addr)tmp);
				/*
				count = 0;
				for (std::map<std::string,Addr>::iterator it=data_map.begin(); it!=data_map.end(); ++it){
					printf("WRITE_HASH: MAP[%d] [%s, %lx]\n", count++, it->first.c_str(), (Addr)it->second);
				}
				*/

				//**************************************
				// we assume that we do not get bad data values, so no checking...
				//**************************************

				//printf("%s:%i: WRITE data[0]: 0x%02X\n", __func__, __LINE__, data[0]);
				rte_memcpy((void*)(tmp + offset), data, size);
				
#ifdef USE_PAXOS                        
		        // the paxos bit
		        memcpy(ht.hash_str, (char*) (data + size), HASH_LENGTH);
		        pax_slave->write_hash(ht, offset, size, (tmp + offset));
#endif
			}

			rte_pktmbuf_free(m);
			break;
		case QUERY:
			port_statistics[dst_port].rx_type[QUERY]++;
			/*
			struct rte_mbuf *reply;
			if (unlikely ((reply = rte_pktmbuf_alloc(l2fwd_pktmbuf_pool)) == NULL)) {
				RTE_LOG(INFO, L2FWD, "ERROR Can not create mbuf for reply to QUERY\n");
				port_statistics[dst_port].alloc_error++;
				break;
			}
			*/
			create_answer_packet(
				m, //reply, 
				dst_port, 
				recv_e_hdr->s_addr.addr_bytes,
				l2fwd_page_size_bits, 
				l2fwd_nb_pages);
			printf_packet(m);

			buffer = tx_buffer[dst_port];
			sent = rte_eth_tx_buffer(dst_port, 0, buffer, m); //reply);
			if (sent)
				port_statistics[dst_port].tx += sent;							
			break;
		case GET:
			// get a value by supplied hash, if it exists
			port_statistics[dst_port].rx_type[GET]++;
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
			tmp_hash.assign((char*) (data), 33);

			// look for the hash
			ptr_map_itter = data_map.find(tmp_hash);
			if(ptr_map_itter == data_map.end()){
				//PRINTF("GET: could not find hash\n");
				create_get_hash_reply_packet(
					m, 
					dst_port, 
					recv_e_hdr->s_addr.addr_bytes,
					0, 
					0,
					rum_hdr->client_size_addr,
					rum_hdr->client_ptr_addr,
					rum_hdr->pak_num);
			}
			else{
				// the hash exists, so the size *must* also
				size_map_itter = data_size_map.find(ptr_map_itter->second);
				create_get_hash_reply_packet(
						m, 
						dst_port,
						recv_e_hdr->s_addr.addr_bytes, 
						ptr_map_itter->second,
						size_map_itter->second,
						rum_hdr->client_size_addr,
						rum_hdr->client_ptr_addr,
						rum_hdr->pak_num 
						);
				//PRINTF("GET: found hash :%lx (%lu bytes)\n", ptr_map_itter->second, size_map_itter->second);
			}

			// and send
			buffer = tx_buffer[dst_port];
			sent = rte_eth_tx_buffer(dst_port, 0, buffer, m); //reply);
			if (sent) {
				port_statistics[dst_port].tx += sent;
			}
			break;
		case GET_REPLY:
			port_statistics[dst_port].rx_type[GET_REPLY]++;
			rte_pktmbuf_free(m);
			break;
		case DATA:
			port_statistics[dst_port].rx_type[DATA]++;
			rte_pktmbuf_free(m);
			break;
		case ANSWER:
			port_statistics[dst_port].rx_type[ANSWER]++;
			rte_pktmbuf_free(m);
			break;
		case ALLOC:
			port_statistics[dst_port].rx_type[ALLOC]++;

			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
			//rte_memcpy(&size, data, sizeof(size_t));
			//size = rum_hdr->data_len;
			memcpy(&size, &rum_hdr->data_len, sizeof(size_t));
			uuid.assign((char*)(data), 33);

			if((tmp = (char*)malloc(size)) == NULL){
				ERROR("ALLOC: silly client is trying to allocate %lu\n", size);
			}

			data_size_map.insert({(Addr)tmp, size});

			// store this memory with the UUID
			if((uuid_iter = uuid_memory_map.find(uuid)) == uuid_memory_map.end()){
				std::vector<Addr> *addrs = new std::vector<Addr>();
				addrs->push_back((Addr)tmp);
				uuid_memory_map.insert({uuid, addrs});
			}
			else{
				((std::vector<Addr>*)uuid_iter->second)->push_back((Addr)tmp);
				//PRINTF("ALLOC: %s has %ld addrs\n", uuid.c_str(), ((std::vector<Addr>*)uuid_iter->second)->size());
			}

			//printf("ALLOC: size %lu @ %lx :: %s\n", size, (uint64_t)tmp, uuid.c_str());
			create_alloc_reply_packet(
				m, 
				dst_port, 
				recv_e_hdr->s_addr.addr_bytes,
				(uint64_t)tmp, 
				size, 
				rum_hdr->client_size_addr,
				rum_hdr->client_ptr_addr,
				rum_hdr->pak_num
				);

			buffer = tx_buffer[dst_port];
			if ((sent = rte_eth_tx_buffer(dst_port, 0, buffer, m))) {
				port_statistics[dst_port].tx += sent;
			}

			total_bytes_allocated += size;
			//printf_packet(m);

#ifdef USE_PAXOS                        
            //paxos bit
            pax_slave->alloc(size, (Addr)tmp);
#endif
			break;
		case DEALLOC:
			port_statistics[dst_port].rx_type[DEALLOC]++;

			// TODO don't need this
			uuid = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);

			if((size_map_itter = data_size_map.find((Addr)rum_hdr->slave_ptr_addr)) != data_size_map.end()){
				
				//PRINTF("DEALLOC: free %lx (%ld bytes)\n", (Addr)rum_hdr->slave_ptr_addr, size_map_itter->second);
				total_bytes_freed += size_map_itter->second;

				// we assume that the DEALLOC_HASH is used for pointers alloced 
				// by ALLOC_HASH, so don't empty the data_map
				free((void*)rum_hdr->slave_ptr_addr);
				data_size_map.erase(size_map_itter);
			}
			else{
				//PRINTF("DEALLOC: did not find %lx\n", (Addr)rum_hdr->slave_ptr_addr);
			}
			rte_pktmbuf_free(m);

#ifdef USE_PAXOS                        
            //paxos bit
            pax_slave->dealloc((Addr)rum_hdr->slave_ptr_addr);
#endif
			break;	
		case ALLOC_HASH:
			port_statistics[dst_port].rx_type[ALLOC_HASH]++;
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

			// space to allocate
			//rte_memcpy(&size, data, sizeof(size_t));
			//size = rum_hdr->data_len;
			memcpy(&size, &rum_hdr->data_len, sizeof(size_t));

			// the hash
			hash.assign(((char*)data), 33);
			//printf("hash is %s\n", hash.c_str());

			// Is this new allocation?
		    if((ptr_itter = data_map.find(hash)) != data_map.end()){
		    	//printf("ALLOC_HASH: found existing ptr\n");
		    	size_map_itter = data_size_map.find((Addr)(ptr_itter->second));
		    	if(size_map_itter != data_size_map.end() && size == size_map_itter->second){
	    			//printf("ALLOC_HASH: Sizes match\n");
			        tmp = (char*)ptr_itter->second;
			    }
			    else{
			    	free((void*)ptr_itter->second);

			    	//printf("ALLOC_HASH: add new ptr\n");
					if((tmp = (char*)malloc(size)) == NULL){
						ERROR("ALLOC_HASH: silly client is trying to allocate %lu\n", size);
					}
					bzero(tmp, size);

					data_map.insert({hash, (Addr)tmp});	
					data_size_map.insert({(Addr)tmp, size});
			    }
		    }
			else{
				//printf("ALLOC_HASH: add new ptr\n");
				if((tmp = (char*)malloc(size)) == NULL){
					ERROR("ALLOC_HASH: silly client is trying to allocate %lu\n", size);
				}
				bzero(tmp, size);

				data_map.insert({hash, (Addr)tmp});	
				data_size_map.insert({(Addr)tmp, size});
			}
			
			// should store the hash and pointer somewhere
			/*
			printf("ALLOC_HASH: size %lu @ %lx\n", size, (Addr)tmp);
			count = 0;
			for (std::map<std::string,Addr>::iterator it=data_map.begin(); it!=data_map.end(); ++it){
				printf("ALLOC_HASH: MAP[%d] [%s, %lx]\n", count++, it->first.c_str(), (Addr)it->second);
			}
			*/

			create_alloc_hash_reply_packet(
				m, 
				dst_port, 
				recv_e_hdr->s_addr.addr_bytes, 
				(Addr)tmp, 
				size, 
				rum_hdr->client_size_addr,
				rum_hdr->client_ptr_addr,
				rum_hdr->pak_num);

			buffer = tx_buffer[dst_port];
			if ((sent = rte_eth_tx_buffer(dst_port, 0, buffer, m))) {
				port_statistics[dst_port].tx += sent;
			}

			total_bytes_allocated += size;
			//printf_packet(m);

#ifdef USE_PAXOS                        
            //paxos bit
            memcpy(ht.hash_str, ((char*)data + sizeof(size_t)), HASH_LENGTH);
            pax_slave->alloc_hash(size, ht, (Addr)tmp);
#endif
			break;
		case DEALLOC_HASH:
			port_statistics[dst_port].rx_type[DEALLOC_HASH]++;
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
			hash.assign(((char*)data), 33);

			if((ptr_itter = data_map.find(hash)) != data_map.end()){
				//printf("DEALLOC_HASH: found ptr\n");
				size_map_itter = data_size_map.find((Addr)(ptr_itter->second));
				if(size_map_itter != data_size_map.end()){
					//printf("DEALLOC_HASH: found size\n");
					total_bytes_freed += size_map_itter->second;
					data_size_map.erase(size_map_itter);	
				}
				free((void*)ptr_itter->second);
				data_map.erase(ptr_itter);
			}
			/*
			printf("DEALLOC_HASH: size %lu @ %lx\n", size_map_itter->second, (Addr)ptr_itter->second);
			count = 0;
			for (std::map<std::string,Addr>::iterator it=data_map.begin(); it!=data_map.end(); ++it){
				printf("DEALLOC_HASH: MAP[%d] [%s, %lx]\n", count++, it->first.c_str(), (Addr)it->second);
			}
			*/
			rte_pktmbuf_free(m);
#ifdef USE_PAXOS                        
            // paxos but
            memcpy(ht.hash_str, ((char*)data + sizeof(size_t)), HASH_LENGTH);
            pax_slave->dealloc_hash(ht);
#endif 
			break;	                      
		default:
			rte_pktmbuf_free(m);
			port_statistics[dst_port].rx_type[UNKNOWN]++;
			break;
	}
}
//------------------------------------------------------------------------------
/* main processing loop */
// this is the per core thread loop
static void l2fwd_main_loop(void){
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	struct rte_mbuf *m;
	int sent;
	unsigned lcore_id;
	uint64_t prev_tsc, diff_tsc, cur_tsc, timer_tsc;
	unsigned i, j, portid, nb_rx;
	struct lcore_queue_conf *qconf;
	const uint64_t drain_tsc = 
			(rte_get_tsc_hz() + US_PER_S - 1) / US_PER_S * BURST_TX_DRAIN_US;
	struct rte_eth_dev_tx_buffer *buffer;

	prev_tsc = 0;
	timer_tsc = 0;

	lcore_id = rte_lcore_id();
	qconf = &lcore_queue_conf[lcore_id];

	if (qconf->n_rx_port == 0) {
		RTE_LOG(INFO, L2FWD, "lcore %u has nothing to do\n", lcore_id);
		return;
	}

	RTE_LOG(INFO, L2FWD, "entering main loop on lcore %u\n", lcore_id);

	for (i = 0; i < qconf->n_rx_port; i++) {
		portid = qconf->rx_port_list[i];
		RTE_LOG(INFO, L2FWD, " -- lcoreid=%u portid=%u\n", lcore_id, portid);
	}

	printf("qconf->n_rx_port %u\n", qconf->n_rx_port);
	while (!force_quit) {

		cur_tsc = rte_rdtsc();

		/*
		 * TX burst queue drain
		 */
		diff_tsc = cur_tsc - prev_tsc;
		if (unlikely(diff_tsc > drain_tsc)) {

			for (i = 0; i < qconf->n_rx_port; i++) {

				portid = l2fwd_dst_ports[qconf->rx_port_list[i]];
				buffer = tx_buffer[portid];

				sent = rte_eth_tx_buffer_flush(portid, 0, buffer);
				if (sent) {
					port_statistics[portid].tx += sent;
					//rte_delay_us_block(DRAIN_WAIT_IN_US);
				}

			}

			/* if timer is enabled */
			if (timer_period > 0) {

				/* advance the timer */
				timer_tsc += diff_tsc;

				/* if timer has reached its timeout */
				if (unlikely(timer_tsc >= timer_period)) {

					/* do this only on master core */
					if (lcore_id == rte_get_master_lcore()) {
						print_stats();
						/* reset the timer */
						timer_tsc = 0;
					}
				}
			}

			prev_tsc = cur_tsc;
		}

		/*
		 * Read packet from RX queues
		 */
		for (i = 0; i < qconf->n_rx_port; i++) {

			portid = qconf->rx_port_list[i];
			nb_rx = rte_eth_rx_burst((uint8_t) portid, 0,
						 pkts_burst, MAX_PKT_BURST);

			port_statistics[portid].rx += nb_rx;
			for (j = 0; j < (nb_rx < PREFETCH_OFFSET ? nb_rx : PREFETCH_OFFSET); j++)
				rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j], void *));
			for (j = 0; j < nb_rx; j++) {
				m = pkts_burst[j];
				if (j+PREFETCH_OFFSET < nb_rx)
					rte_prefetch0(rte_pktmbuf_mtod(pkts_burst[j+PREFETCH_OFFSET], void *));
				l2fwd_simple_forward(m, portid);
			}
		}
	}
}
//------------------------------------------------------------------------------
static int l2fwd_launch_one_lcore(__attribute__((unused)) void *dummy){
	l2fwd_main_loop();
	return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// THe folowing are basically parsing functions
//------------------------------------------------------------------------------
/* display usage */
static void l2fwd_usage(const char *prgname){
	printf("Usage: %s [EAL options] -- -m M -s S -p PORTMASK [-q NQ]\n"
			"  -m M: mem pool size in bits. 32 is 4GB.\n"
			"  -s S: page size in bits. 12 is 4 kb.\n"
	       "  -p PORTMASK: hexadecimal bitmask of ports to configure\n"
	       "  -q NQ: number of queue (=ports) per lcore (default is 1)\n"
		   "  -T PERIOD: statistics will be refreshed each PERIOD seconds (0 to disable, 10 default, 86400 maximum)\n"
		   "  --[no-]mac-updating: Enable or disable MAC addresses updating (enabled by default)\n"
		   "      When enabled:\n"
		   "       - The source MAC address is replaced by the TX port MAC address\n"
		   "       - The destination MAC address is replaced by 02:00:00:00:00:TX_PORT_ID\n",
	       prgname);
}
//------------------------------------------------------------------------------
static void l2fwd_init_data_arg_check(const char *prgname){
	if (l2fwd_data_pool_bits == 0) {
		l2fwd_usage(prgname);
		rte_exit(EXIT_FAILURE,
			"Data pool bits is Zero. Please set correctly.\n");
	}

	if (l2fwd_page_size_bits == 0) {
		l2fwd_usage(prgname);
		rte_exit(EXIT_FAILURE,
			"Page size bits is Zero. Please set correctly.\n");
	}		
}
//------------------------------------------------------------------------------
static int l2fwd_parse_data_pool_bits(const char *arg) {
	char *end = NULL;
	unsigned long n;

	/* parse decimal string */
	n = strtoul(arg, &end, 10);
	if ((arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0) { return 0; }
	return n;
}
//------------------------------------------------------------------------------
static int l2fwd_parse_page_size_bits(const char *arg){
	char *end = NULL;
	unsigned long n;

	/* parse decimal string */
	n = strtoul(arg, &end, 10);
	if ((arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0) { return 0; }
	return n;
}
//------------------------------------------------------------------------------
static int l2fwd_parse_portmask(const char *portmask){
	char *end = NULL;
	unsigned long pm;

	/* parse hexadecimal string */
	pm = strtoul(portmask, &end, 16);
	if ((portmask[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;

	if (pm == 0) { return -1; }
	return pm;
}
//------------------------------------------------------------------------------
static unsigned int l2fwd_parse_nqueue(const char *q_arg) {
	char *end = NULL;
	unsigned long n;

	/* parse hexadecimal string */
	n = strtoul(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return 0;
	if (n == 0)
		return 0;
	if (n >= MAX_RX_QUEUE_PER_LCORE)
		return 0;

	return n;
}
//------------------------------------------------------------------------------
static int l2fwd_parse_timer_period(const char *q_arg){
	char *end = NULL;
	int n;

	/* parse number string */
	n = strtol(q_arg, &end, 10);
	if ((q_arg[0] == '\0') || (end == NULL) || (*end != '\0'))
		return -1;
	if (n >= MAX_TIMER_PERIOD)
		return -1;

	return n;
}
//------------------------------------------------------------------------------
static const char short_options[] =
	"p:"  /* portmask */
	"q:"  /* number of queues */
	"T:"  /* timer period */
	"s:"  /* page size in bits */
	"m:"  /* mem pool size in bits */
	;

#define CMD_LINE_OPT_MAC_UPDATING "mac-updating"
#define CMD_LINE_OPT_NO_MAC_UPDATING "no-mac-updating"

enum {
	/* long options mapped to a short option */

	/* first long only option value must be >= 256, so that we won't
	 * conflict with short options */
	CMD_LINE_OPT_MIN_NUM = 256,
};

static const struct option lgopts[] = {
	{ CMD_LINE_OPT_MAC_UPDATING, no_argument, &mac_updating, 1},
	{ CMD_LINE_OPT_NO_MAC_UPDATING, no_argument, &mac_updating, 0},
	{NULL, 0, 0, 0}
};
//------------------------------------------------------------------------------
/* Parse the argument given in the command line of the application */
static int l2fwd_parse_args(int argc, char **argv) {
	int opt, ret, timer_secs;
	char **argvopt;
	int option_index;
	char *prgname = argv[0];

	argvopt = argv;

	while ((opt = getopt_long(argc, argvopt, short_options,
				  lgopts, &option_index)) != EOF) {

		switch (opt) {
		/* mem_pool_size_bits */
		case 'm':
			l2fwd_data_pool_bits = l2fwd_parse_data_pool_bits(optarg);
			if (l2fwd_data_pool_bits == 0) {
				printf("invalid mem pool size in bits\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* page_size_bits */
		case 's':
			l2fwd_page_size_bits = l2fwd_parse_page_size_bits(optarg);
			if (l2fwd_page_size_bits == 0) {
				printf("invalid page size in bits\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* portmask */
		case 'p':
			l2fwd_enabled_port_mask = l2fwd_parse_portmask(optarg);
			if (l2fwd_enabled_port_mask == 0) {
				printf("invalid portmask\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* nqueue */
		case 'q':
			l2fwd_rx_queue_per_lcore = l2fwd_parse_nqueue(optarg);
			if (l2fwd_rx_queue_per_lcore == 0) {
				printf("invalid queue number\n");
				l2fwd_usage(prgname);
				return -1;
			}
			break;

		/* timer period */
		case 'T':
			timer_secs = l2fwd_parse_timer_period(optarg);
			if (timer_secs < 0) {
				printf("invalid timer period\n");
				l2fwd_usage(prgname);
				return -1;
			}
			timer_period = timer_secs;
			break;

		/* long options */
		case 0:
			break;

		default:
			l2fwd_usage(prgname);
			return -1;
		}
	}

	if (optind >= 0)
		argv[optind-1] = prgname;

	ret = optind-1;
	optind = 1; /* reset getopt lib */
	return ret;
}
//------------------------------------------------------------------------------
/* Check the link status of all ports in up to 9s, and print them finally */
static void l2fwd_check_all_ports_link_status(uint8_t port_num, uint32_t port_mask){
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		if (force_quit)
			return;
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if (force_quit)
				return;
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
						"Mbps - %s\n", (uint8_t)portid,
						(unsigned)link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
						(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// given a path, return a string representing that path
struct path_string{
    std::string operator()(const boost::filesystem::directory_entry& entry) const {
        return entry.path().string();//.leaf().string();
    }
};

struct path_leaf_string{
    std::string operator()(const boost::filesystem::directory_entry& entry) const {
        return entry.path().leaf().string();
    }
};
//------------------------------------------------------------------------------
#define BACKUP_FILE_PATH "/home/paul/RUM/DATA/"
void write_to_file(std::string name, char *ptr, size_t size){
	std::fstream dst_file;
	std::string fname = BACKUP_FILE_PATH + name;

	dst_file.open(fname, std::ios::out | std::ios::binary);
	dst_file.write((char*)&size, sizeof(size_t));
	dst_file.write(ptr, size);
	dst_file.close();
}
//------------------------------------------------------------------------------
void read_from_file(std::string name, char *ptr, size_t *size){
	std::fstream file;

	file.open(name, std::ios::in | std::ios::binary);
	file.read((char*)size, sizeof(size_t));

	if((ptr = (char*)malloc(*size)) == NULL){
		ERROR("READ_FILE: malloc failed\n");
	}

	file.read(ptr, *size);
	file.close();
}
//------------------------------------------------------------------------------
void backup(){
	printf("Current Memory Locations\n");
	std::map<std::string, Addr>::iterator h_it;
	std::map<Addr, size_t>::iterator it;

	// TODO: should probably create a reverse hash table to make this faster
	//		 or just use structures....
	int count = 0;
	for(it = data_size_map.begin() ; it != data_size_map.end() ; it++){
		bool found = false;
		for (h_it = data_map.begin(); h_it != data_map.end(); h_it++){
			if (h_it->second == it->first){
				printf("[%d] <%s> %lx (%lu bytes)\n", count++, h_it->first.c_str(), it->first, it->second);
				write_to_file(h_it->first, (char*)it->first, it->second);
				found = true;
				break;
			}
		}
		    
		if(!found){
			printf("[%d] <> %lx (%lu bytes)\n", count++, it->first, it->second);	
			write_to_file("unknown_" + std::to_string(count), (char*)it->first, it->second);
		}
	}

	// assume that we do not need update the accoutning structures....
}
//------------------------------------------------------------------------------
void restore(){
	std::vector<std::string>  file_paths;
	std::vector<std::string>  files;
	size_t 	size;
	char*	ptr = NULL;
	int 	count = 0;

	// get the files from the directory
    boost::filesystem::path p (BACKUP_FILE_PATH);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    transform(start, end, std::back_inserter(file_paths), path_string());

 	boost::filesystem::path p2 (BACKUP_FILE_PATH);
    boost::filesystem::directory_iterator start2(p2);
    boost::filesystem::directory_iterator end2;
    transform(start2, end2, std::back_inserter(files), path_leaf_string());

    count = 0;
    for(std::string s : file_paths){
		printf("[%d/%ld] %s\n", count++, file_paths.size(), s.c_str());
    }

    count = 0;
    for(std::string s : files){
		printf("[%d/%ld] %s\n", count++, files.size(), s.c_str());
    }
    count = 0;
    printf("done\n");

    for(std::string s : file_paths){
    	read_from_file(s, ptr, &size);

    	// add to the store
    	if(s.find("unknown_") == std::string::npos){
    		data_map.insert({files.at(count), (Addr)ptr});		
    	}
		data_size_map.insert({(Addr)ptr, size});
		count++;
    }

//debug
#if 1
	std::map<std::string, Addr>::iterator h_it;
	std::map<Addr, size_t>::iterator it;
    count = 0;
	for(it = data_size_map.begin() ; it != data_size_map.end() ; it++){
		bool found = false;
		for (h_it = data_map.begin(); h_it != data_map.end(); h_it++){
			if (h_it->second == it->first){
				printf("[%d] <%s> %lx (%lu bytes)\n", count++, h_it->first.c_str(), it->first, it->second);
				found = true;
				break;
			}
		}
		    
		if(!found){
			printf("[%d] <> %lx (%lu bytes)\n", count++, it->first, it->second);	
		}
	}
#endif
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static void signal_handler(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n", signum);
		force_quit = true;

		// write memory to disk...
		backup();

		hb->stop_daemon();
	}
}
//------------------------------------------------------------------------------
void callback_function(void* value, size_t len){
    PRINTF("CALLBACK :: \n");
    //PRINTF("CALLBACK :: %d\n", value);
    size_t demarshaled_bytes = 0;
    slave_paxos_msg *msg = slave_paxos_msg::demarshal((char*)value, len, &demarshaled_bytes);
    msg->dump();
}
//------------------------------------------------------------------------------
int main(int argc, char **argv)
{  
#ifdef USE_PAXOS
    ////////////////////////////////////////
    // PAXOS
    // somehow get the slaves in the paxos network
    std::shared_ptr<Node> alpha = std::make_shared<Node>("192.168.0.1", 45670);
    std::shared_ptr<Node> beta  = std::make_shared<Node>("192.168.0.2", 45670);
    std::shared_ptr<Node> gamma = std::make_shared<Node>("192.168.0.3", 45670);
    std::vector<std::shared_ptr<Node>> peers    = {alpha, beta, gamma};
    
    // setup our paxos communicator
    pax_slave = new SlavePaxos(peers);
    int choice = 0;
    
    std::shared_ptr<Node> client_addr = std::make_shared<Node>(peers.at(choice)->get_ip(), 45671);    

    Node* nodes[] = {alpha.get(), beta.get(), gamma.get()};
    
    pax_backup_filename     = "./paxos_" + std::to_string(choice) + ".bkk";
    
    // we start a paxos slave
    pax_inst = new paxos_instance(peers.at(choice), peers, pax_backup_filename, &callback_function);
    pax_inst->start();
    pax_daemon_thread           = std::thread(&paxos_instance::daemon, pax_inst, *nodes[choice]);
    pax_client_daemon_thread    = std::thread(&paxos_instance::client_daemon, pax_inst, *(client_addr.get()));
    TODO("join on this thread at some point\n");
#endif
    ////////////////////////////////////////
        
    ////////////////////////////////////////////////////////////////////////////
    // RESTORE
	// before we start the threads to listen on the network, restore data
	restore();
	////////////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////////////
	// HEARTBEAT
	// Start the slave part of the Heartbeat
	//std::string slave_ip("172.22.0.31");
	std::string slave_ip = get_ip_address();
	hb = new Heartbeat(slave_ip, user_callback_function);
	hb->start_daemon();
	////////////////////////////////////////////////////////////////////////////

	struct lcore_queue_conf *qconf;
	struct rte_eth_dev_info dev_info;
	int ret;
	uint8_t nb_ports;
	uint8_t nb_ports_available;
	uint8_t portid, last_port;
	unsigned lcore_id, rx_lcore_id;
	unsigned nb_ports_in_mask = 0;

	printf("MAX_PKT_BURST %u\n", MAX_PKT_BURST);
	printf("BURST_TX_DRAIN_US %u\n", BURST_TX_DRAIN_US);

	struct rte_eth_conf port_conf;
	dpdk_init_port_conf(&port_conf);

	/* init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");
	argc -= ret;
	argv += ret;

	force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* parse application arguments (after the EAL ones) */
	ret = l2fwd_parse_args(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid L2FWD arguments\n");

	//printf("MAC updating %s\n", mac_updating ? "enabled" : "disabled");

	/* convert to number of cycles */
	timer_period *= rte_get_timer_hz();

	/* create the mbuf pool */
	l2fwd_pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF,
//		MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, // TODO jumbo
		MEMPOOL_CACHE_SIZE, 0, 8192, // TODO jumbo
		rte_socket_id());
	if (l2fwd_pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

	nb_ports = rte_eth_dev_count();
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* reset l2fwd_dst_ports */
	printf("RTE_MAX_ETHPORTS %u\n", RTE_MAX_ETHPORTS);
	for (portid = 0; portid < RTE_MAX_ETHPORTS; portid++)
		l2fwd_dst_ports[portid] = 0;
	last_port = 0;

	/*
	 * Each logical core is assigned a dedicated TX queue on each port.
	 */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		if (nb_ports_in_mask % 2) {
			l2fwd_dst_ports[portid] = last_port;
			l2fwd_dst_ports[last_port] = portid;
		}
		else
			last_port = portid;

		nb_ports_in_mask++;

		rte_eth_dev_info_get(portid, &dev_info);
	}
	if (nb_ports_in_mask % 2) {
		printf("Notice: odd number of ports in portmask.\n");
		l2fwd_dst_ports[last_port] = last_port;
	}

	rx_lcore_id = 0;
	qconf = NULL;

	/* Initialize the port/queue configuration of each logical core */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;

		/* get the lcore_id for this port */
		while (rte_lcore_is_enabled(rx_lcore_id) == 0 ||
		       lcore_queue_conf[rx_lcore_id].n_rx_port ==
		       l2fwd_rx_queue_per_lcore) {
			rx_lcore_id++;
			if (rx_lcore_id >= RTE_MAX_LCORE)
				rte_exit(EXIT_FAILURE, "Not enough cores\n");
		}

		if (qconf != &lcore_queue_conf[rx_lcore_id])
			/* Assigned a new logical core in the loop above. */
			qconf = &lcore_queue_conf[rx_lcore_id];

		qconf->rx_port_list[qconf->n_rx_port] = portid;
		qconf->n_rx_port++;
		printf("Lcore %u: RX port %u\n", rx_lcore_id, (unsigned) portid);
	}

	nb_ports_available = nb_ports;
	printf("nb_ports_available %u\n", nb_ports_available);

	l2fwd_init_data_arg_check(argv[0]);
	l2fwd_init_data(0);

	/* Initialise each port */
	for (portid = 0; portid < nb_ports; portid++) {
		/* skip ports that are not enabled */
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0) {
			printf("Skipping disabled port %u\n", (unsigned) portid);
			nb_ports_available--;
			continue;
		}
		/* init port */
		printf("Initializing port %u... ", (unsigned) portid);
		fflush(stdout);
		ret = rte_eth_dev_configure(portid, 1, 1, &port_conf);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n",
				  ret, (unsigned) portid);

		rte_eth_macaddr_get(portid,&l2fwd_ports_eth_addr[portid]);

		/* init one RX queue */
		fflush(stdout);
		ret = rte_eth_rx_queue_setup(portid, 0, nb_rxd,
					     rte_eth_dev_socket_id(portid),
					     NULL,
					     l2fwd_pktmbuf_pool);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n",
				  ret, (unsigned) portid);

		/* init one TX queue on each port */
		fflush(stdout);
		ret = rte_eth_tx_queue_setup(portid, 0, nb_txd,
				rte_eth_dev_socket_id(portid),
				NULL);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n",
				ret, (unsigned) portid);

		/* Initialize TX buffers */
		tx_buffer[portid] = (struct rte_eth_dev_tx_buffer *)rte_zmalloc_socket("tx_buffer",
				RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0,
				rte_eth_dev_socket_id(portid));
		if (tx_buffer[portid] == NULL)
			rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n",
					(unsigned) portid);

		rte_eth_tx_buffer_init(tx_buffer[portid], MAX_PKT_BURST);

		ret = rte_eth_tx_buffer_set_err_callback(tx_buffer[portid],
				//rte_eth_tx_buffer_count_callback,
				tx_buffer_retransmit_callback,
				//&port_statistics[portid].dropped);
				NULL);
		if (ret < 0)
				rte_exit(EXIT_FAILURE, "Cannot set error callback for "
						"tx buffer on port %u\n", (unsigned) portid);
	
		/* Start device */
		ret = rte_eth_dev_start(portid);
		if (ret < 0)
			rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n",
				  ret, (unsigned) portid);

		printf("done: \n");

		rte_eth_promiscuous_enable(portid);

		printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
				(unsigned) portid,
				l2fwd_ports_eth_addr[portid].addr_bytes[0],
				l2fwd_ports_eth_addr[portid].addr_bytes[1],
				l2fwd_ports_eth_addr[portid].addr_bytes[2],
				l2fwd_ports_eth_addr[portid].addr_bytes[3],
				l2fwd_ports_eth_addr[portid].addr_bytes[4],
				l2fwd_ports_eth_addr[portid].addr_bytes[5]);

		/* initialize port stats */
		memset(&port_statistics, 0, sizeof(port_statistics));
	}

	if (!nb_ports_available) {
		l2fwd_usage(argv[0]);
		rte_exit(EXIT_FAILURE,
			"All available ports are disabled. Please set portmask.\n");
	}

	l2fwd_check_all_ports_link_status(nb_ports, l2fwd_enabled_port_mask);

	ret = 0;
	/* launch per-lcore init on every lcore */
	rte_eal_mp_remote_launch(l2fwd_launch_one_lcore, NULL, CALL_MASTER);
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}

	for (portid = 0; portid < nb_ports; portid++) {
		if ((l2fwd_enabled_port_mask & (1 << portid)) == 0)
			continue;
		printf("Closing port %d...", portid);
		rte_eth_dev_stop(portid);
		rte_eth_dev_close(portid);
		printf(" Done\n");
	}
	printf("Bye...\n");

	return ret;
}
//------------------------------------------------------------------------------