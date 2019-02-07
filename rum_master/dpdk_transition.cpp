/******************************************************************************
CPP test

02-20 C++11 compilation using cmake working.
02-19 Write-read verified. See "[message]" in menu.
******************************************************************************/

#include "dpdk_transition.h"

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include <cstdint>
#include <cinttypes>

#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_lcore.h>
#include <rte_malloc.h>
#include <rte_mbuf.h>
#include <rte_pdump.h>
#include <rte_ring.h>

//#include <stdbool.h>

// sleep
#include <unistd.h>

//#define DEBUG
#include "debug.h"
#include "ipv6.h"
#include "nic.h"
#include "packet.h"

#ifdef MEASUREMENTS
#include "request.hpp"
#include "estimator.h"
#include "histogram.hpp"
#include "mathext.hpp"
#endif

// to kill the warnings!
#define igr(x) {__typeof__(x) __attribute__((unused)) d=(x);} 

//#define RX_QUEUE_SIZE (1UL << 9)
//#define TX_QUEUE_SIZE (1UL << 11)
#define DESCRIPTOR_FACTOR (8)
#define RTE_TEST_RX_DESC_DEFAULT (128*DESCRIPTOR_FACTOR)
#define RTE_TEST_TX_DESC_DEFAULT (512*DESCRIPTOR_FACTOR)

#define BURST_SIZE_TX 32 // 32
//#define RX_RING_SIZE 8192
#define TX_RING_SIZE 65536

#define SEND_BURST_RETRY 10000
/* allow max jumbo frame 9.5 KB */
#define	JUMBO_FRAME_MAX_SIZE	0x2600
#define PREFETCH_OFFSET 3
#define ROUNDTRIP_WAIT_US (0)
#define RETURN_WAIT_US (1)
#define LCORE_TX_WAIT_US (0)
#define SET_ASYNC_WAIT_US (1)

// The number of elements in the mbuf pool. The optimum size (in terms of memory
// usage) for a mempool is when n is a power of two minus one: n = (2^q - 1).
#define NUM_MBUFS (1UL << 14)
//#define NUM_MBUFS 8192
#define MBUF_CACHE_SIZE 256
#define BURST_SIZE 128 // 128

// Number of cores needed.
// Prints out a warning message when this library sees more.
#define LCORE_NEEDED 4
// Window size must be big enough so that the transport ID does not wrap-around
// the transport window.
// For performance, choose a power of 2
#define TRANSPORT_WINDOW_SIZE 131072
//#define TRANSPORT_TX_RING_SIZE 65536

#define DEV_TX_OFFLOAD_MT_LOCKFREE      0x00004000

#define NUM_SLAVE_NODES_IN_BITS 1

struct rte_mempool *mbuf_pool;

unsigned nb_ports;

size_t l2fwd_data_pool_bits; // 32 = 4GB
size_t l2fwd_data_pool_size;
uint8_t *l2fwd_data_pool;
size_t l2fwd_page_size_bits; // 12 = 4KB
size_t l2fwd_page_size;
size_t l2fwd_nb_pages;
size_t slave_page_size_bits;
size_t slave_page_size;

uint8_t *l2fwd_message;

std::atomic<uint32_t> packet_number;
//TODO track queue size
bool packet_received[RTE_MAX_ETHPORTS][TRANSPORT_WINDOW_SIZE] __rte_cache_aligned;

static bool lcore_tx_running[RTE_MAX_ETHPORTS] __rte_cache_aligned;
static bool lcore_rx_running[RTE_MAX_ETHPORTS] __rte_cache_aligned;

struct packet_stats {
    uint32_t total;
    uint32_t dropped;
    uint32_t forwarded;
    uint32_t retried;
    uint32_t waited;
    uint32_t type[END_PACKET];
} __rte_cache_aligned;

struct port_stats {
    struct packet_stats tx;
    struct packet_stats rx;
} __rte_cache_aligned;

struct port_stats port_statistics[RTE_MAX_ETHPORTS] __rte_cache_aligned;
struct port_stats port_statistics_total __rte_cache_aligned;

#ifdef MEASUREMENTS
std::vector< UIntHistogram > HISTOGRAM(2, UIntHistogram(32, "lcore_tx"));
Request<uint64_t, false> G_REQUEST;
std::vector< Request<uint64_t, false> > REQUESTS(2);
#endif 

struct rte_ring *TX_RING_[RTE_MAX_ETHPORTS];

typedef std::chrono::high_resolution_clock Clock;
typedef std::chrono::high_resolution_clock::time_point TimePoint;
typedef std::vector< std::pair< size_t, TimePoint> > TimeTrace;
static std::vector< TimeTrace > DPDK_time_points;
static std::vector< std::pair< size_t, size_t > > DPDK_time_sizes;
//------------------------------------------------------------------------------    
#define ETHER_ADDR_LEN_BYTES 6
bool compare_ether_hdr(struct ether_addr first, unsigned char* b){
	uint8_t *a = first.addr_bytes;
    //uint8_t *b = second.addr_bytes;
    
    //printf("== Ethernet ==\n");
   // printf("first  MAC %02X %02X %02X %02X %02X %02X\n", a[0], a[1], a[2], a[3], a[4], a[5]);
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
bool compare_ether_hdr(unsigned char* a, unsigned char* b){
    
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
// This is a hack to match a destination MAC addr to a port
uint16_t get_port_from_addr(unsigned char *a){

    /*
    Location A = {{"172.22.0.33"}, {0XA0, 0X36, 0X9F, 0X1A, 0X2A, 0X14}};
    Location B = {{"172.22.0.31"}, {0XA0, 0X36, 0X9F, 0X1A, 0X13, 0X0C}};
    Location C = {{"172.22.0.34"}, {0XA0, 0X36, 0X9F, 0X7F, 0X9A, 0X44}};
    Location C = {{"172.22.0.35"}, {0XA0, 0X36, 0X9F, 0X7F, 0X9A, 0X34}};
    

    if( compare_ether_hdr(A.mac_addr, a) ||
        compare_ether_hdr(B.mac_addr, a) ||
        compare_ether_hdr(C.mac_addr, a)){
        return 0;
    }
    else{
        return -1;
    }
    */

    // assuming that everything is just going on port 0...
    return 0;

#if 0
    int nb_ports = rte_eth_dev_count();
    struct ether_addr addr;

    printf("There are %d ports\n", nb_ports);
    int i;
    for(i = 0 ; i < nb_ports ; i++){
        rte_eth_macaddr_get(i, &addr);
       /* 
        printf("Port %u MAC: %02X %02X %02X %02X %02X %02X\n",
                    (unsigned)port,
                    addr.addr_bytes[0], addr.addr_bytes[1],
                    addr.addr_bytes[2], addr.addr_bytes[3],
                    addr.addr_bytes[4], addr.addr_bytes[5]);
        */
        if(compare_ether_hdr(addr, a)){
           return i; 
        }
    }
    return -1;
#endif
}
//------------------------------------------------------------------------------
// have we received a packet with id 'transport_id'
bool dpdkIsPacketReceived(size_t transport_id){
    //printf("%ld:: [%d] = %d, [%d] = %d\n",transport_id, 0, packet_received[0][transport_id], 1, packet_received[1][transport_id]);
    for (size_t i = 0; i < nb_ports; i++)
        if (packet_received[i][transport_id])
            return true;
    return false;
}
//------------------------------------------------------------------------------
// clear the flag for the set 'transport_id'
void dpdkSetPacketReceivedFalse(size_t transport_id){
    for (size_t i = 0; i < nb_ports; i++)
        packet_received[i][transport_id] = false;
}
//------------------------------------------------------------------------------
// wait for packet 'pak_num'  to be received
void dpdkWaitFor(uint64_t pak_num) {
    size_t transport_id = pak_num % TRANSPORT_WINDOW_SIZE;

    //PRINTF("WAIT_FOR_PKT: %ld <%ld>\n", pak_num, transport_id);
    while (!dpdkIsPacketReceived(transport_id)) {
        if (RETURN_WAIT_US)
            std::this_thread::sleep_for(std::chrono::microseconds(RETURN_WAIT_US));
    }
    //printf("pak %lu transport %lu received %i\n", pak_num, transport_id, packet_received[transport_id]);
    dpdkSetPacketReceivedFalse(transport_id);
}
//------------------------------------------------------------------------------
// tracking software for when ports have received packtes from slaves
int dpdk_init_transport(void) {
    for (size_t slave = 0; slave < nb_ports; slave++) {
        for (size_t i = 0; i < TRANSPORT_WINDOW_SIZE; i++){
            packet_received[slave][i] = false;
        }
    }
    return 0;
}
//------------------------------------------------------------------------------
// assign a single lunp of host memory to dpdk
void dpdkInitHostMemory(char *host_memory) {
    TODO("change the single memory access\n");
    l2fwd_data_pool = (uint8_t *)host_memory;
    dprintf("l2fwd_data_pool 0x%lx\n", (size_t)l2fwd_data_pool);
    dprintf("Route 1 addr 0x%lx\n", (size_t)(l2fwd_data_pool+l2fwd_data_pool_size));
}
//------------------------------------------------------------------------------ 
// set some configuration parameters
void dpdk_init_data(size_t numa_socket) {
	l2fwd_data_pool_size = 1UL << l2fwd_data_pool_bits;
	l2fwd_page_size = 1UL << l2fwd_page_size_bits;
	l2fwd_nb_pages = 1UL << (l2fwd_data_pool_bits - l2fwd_page_size_bits);
	dprintf("l2fwd_data_pool_bits %lu\n", l2fwd_data_pool_bits);
	dprintf("l2fwd_data_pool_size %lu\n", l2fwd_data_pool_size);
	dprintf("l2fwd_page_size_bits %lu\n", l2fwd_page_size_bits);
	dprintf("l2fwd_page_size %lu\n", l2fwd_page_size);
	dprintf("l2fwd_nb_pages %lu\n", l2fwd_nb_pages);

    dprintf("numa_socket %lu\n", numa_socket);
    /*
	l2fwd_data_pool = (uint8_t *)rte_zmalloc_socket("DATA_POOL", l2fwd_data_pool_size, 0, numa_socket);
	if (l2fwd_data_pool == NULL) {
		rte_exit(EXIT_FAILURE, "Cannot init data pool\n");
	}
    */

   slave_page_size_bits = l2fwd_page_size_bits - NUM_SLAVE_NODES_IN_BITS;
   slave_page_size = 1UL << slave_page_size_bits;
   dprintf("slave_page_size_bits %lu\n", slave_page_size_bits);
   dprintf("slave_page_size %lu\n", slave_page_size);
}
//------------------------------------------------------------------------------
// clear the stats
void dpdk_init_port_stats(void) {
    size_t i;
    for (i = 0; i < RTE_MAX_ETHPORTS; i++)
        memset((char *)(&port_statistics[i]), 0, sizeof(struct port_stats));
    memset((char *)(&port_statistics_total), 0, sizeof(struct port_stats));
}
//------------------------------------------------------------------------------
// dump stats
void dpdk_print_port_stats(struct port_stats *s) {
    size_t tx_total         = s->tx.total;
    size_t tx_dropped       = s->tx.dropped;
    size_t tx_forwarded     = s->tx.forwarded;
    size_t tx_retried       = s->tx.retried;
    size_t tx_waited        = s->tx.waited;
    size_t tx_UNKNOWN       = s->tx.type[UNKNOWN];
    size_t tx_READ          = s->tx.type[READ];
    size_t tx_WRITE         = s->tx.type[WRITE];
    size_t tx_WRITE_HASH    = s->tx.type[WRITE_HASH];
    size_t tx_GET           = s->tx.type[GET];
    size_t tx_GET_REPLY     = s->tx.type[GET_REPLY];
    size_t tx_QUERY         = s->tx.type[QUERY];
    size_t tx_DATA          = s->tx.type[DATA];
    size_t tx_ANSWER        = s->tx.type[ANSWER];
    size_t tx_ALLOC         = s->tx.type[ALLOC];
    size_t tx_ALLOC_HASH    = s->tx.type[ALLOC_HASH];
    size_t rx_total         = s->rx.total;
    size_t rx_dropped       = s->rx.dropped;
    size_t rx_forwarded     = s->rx.forwarded;
    size_t rx_retried       = s->rx.retried;
    size_t rx_waited        = s->rx.waited;
    size_t rx_UNKNOWN       = s->rx.type[UNKNOWN];
    size_t rx_READ          = s->rx.type[READ];
    size_t rx_WRITE          = s->rx.type[WRITE];
    size_t rx_WRITE_HASH    = s->rx.type[WRITE_HASH];
    size_t rx_GET           = s->rx.type[GET];
    size_t rx_GET_REPLY     = s->rx.type[GET_REPLY];
    size_t rx_QUERY         = s->rx.type[QUERY];
    size_t rx_DATA          = s->rx.type[DATA];
    size_t rx_ANSWER        = s->rx.type[ANSWER];
    size_t rx_ALLOC         = s->rx.type[ALLOC];
    size_t rx_ALLOC_HASH    = s->rx.type[ALLOC_HASH];
	printf("\n                    Transmitter              Receiver"
           "\n      Total %21lu %21lu"
           "\n    Dropped %21lu %21lu"
           "\n  Forwarded %21lu %21lu"
           "\n    Retried %21lu %21lu"
           "\n     Waited %21lu %21lu"
           "\n"
           "\n    UNKNOWN %21lu %21lu"
           "\n       READ %21lu %21lu"
           "\n      WRITE %21lu %21lu"
           "\n WRITE_HASH %21lu %21lu"
           "\n        GET %21lu %21lu"
           "\n  GET_REPLY %21lu %21lu"
           "\n      QUERY %21lu %21lu"
           "\n       DATA %21lu %21lu"
           "\n     ANSWER %21lu %21lu"
           "\n      ALLOC %21lu %21lu"
           "\n  ALOC_HASH %21lu %21lu",
           tx_total,        rx_total,
           tx_dropped,      rx_dropped,
           tx_forwarded,    rx_forwarded,
           tx_retried,      rx_retried,
           tx_waited,       rx_waited,
           tx_UNKNOWN,      rx_UNKNOWN,
           tx_READ,         rx_READ,
           tx_WRITE,        rx_WRITE,
           tx_WRITE_HASH,   rx_WRITE_HASH,
           tx_GET,          rx_GET,
           tx_GET_REPLY,    rx_GET_REPLY,
           tx_QUERY,        rx_QUERY,
           tx_DATA,         rx_DATA,
           tx_ANSWER,       rx_ANSWER,
           tx_ALLOC,        rx_ALLOC,
           tx_ALLOC_HASH,   rx_ALLOC_HASH);
}
//------------------------------------------------------------------------------
void dpdk_print_port_stats_all(void) {
    size_t i;

	printf("\nPort statistics =====================================");
    for (i = 0; i < nb_ports; i++) {
	    printf("\nPort %lu ----------------------------------------------", i);
        dpdk_print_port_stats(&port_statistics[i]);
    }
	printf("\nAggregate statistics ================================");
    dpdk_print_port_stats(&port_statistics_total);
	printf("\n=====================================================\n");
    fflush(stdout);
}
//------------------------------------------------------------------------------
uint64_t dpdk_stats_get_tx_total(void){
    return port_statistics_total.tx.total;
}
//------------------------------------------------------------------------------
uint64_t dpdk_stats_get_rx_total(void){
    return port_statistics_total.rx.total;
}
//------------------------------------------------------------------------------
// record statistics on the outgoing packets
uint16_t dpdk_classify_tx(
    uint16_t port,
    uint16_t qidx __rte_unused,
	struct rte_mbuf **pkts, 
    uint16_t nb_pkts,
    void *user_param __rte_unused
) {
    /*for debugging
    static int once = 1;
    if (once) {
        dprintf("Core ID %u\n", rte_lcore_id());
        once = 0;
    }
    */

    size_t i;
    struct packet_stats *s = &port_statistics[port].tx;
    struct packet_stats *total = &port_statistics_total.tx;
    s->total += nb_pkts;
    total->total += nb_pkts;
	for (i = 0; i < nb_pkts; i++)
    {
        //printf_packet(pkts[i]); // DEBUG
        RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(pkts[i], RUM_HDR_T*, ETHER_HDR_LEN);
        uint8_t type = rum_hdr->type;
        if (type < END_PACKET) {
            s->type[type]++;
            total->type[type]++;
        } else {
            s->type[UNKNOWN]++;
            total->type[UNKNOWN]++;
        }
    }
	return nb_pkts;
}
//------------------------------------------------------------------------------
// record statistics on the incomming packets
uint16_t dpdk_classify_rx(
    uint16_t port,
    uint16_t qidx __rte_unused,
	struct rte_mbuf **pkts, 
    uint16_t nb_pkts,
    uint16_t max_pkts __rte_unused,
    void *user_param __rte_unused
) {
    /*for debugging
    static int once = 1;
    if (once) {
        dprintf("Core ID %u\n", rte_lcore_id());
        once = 0;
    }
    */

    size_t i;
    struct packet_stats *s = &port_statistics[port].rx;
    struct packet_stats *total = &port_statistics_total.rx;
    s->total += nb_pkts;
    total->total += nb_pkts;
	for (i = 0; i < nb_pkts; i++)
    {
        //printf_packet(pkts[i]); // DEBUG
        RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(pkts[i], RUM_HDR_T*, ETHER_HDR_LEN);
        uint8_t type = rum_hdr->type;
        if (type < END_PACKET) {
            s->type[type]++;
            total->type[type]++;
        } else {
            s->type[UNKNOWN]++;
            total->type[UNKNOWN]++;
        }
    }
	return nb_pkts;
}
//------------------------------------------------------------------------------
void dpdk_show_capability(void){
    TODO("dpdk_show_capability\n");
    /*
    size_t i;
    RTE_ETH_FOREACH_DEV(i) {
        struct rte_eth_dev_info dev_info;
        memset(&dev_info, 0, sizeof(struct rte_eth_dev_info));
        rte_eth_dev_info_get(i, &dev_info);
        // Check if it is safe ask worker to tx. 
        if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MT_LOCKFREE)
            dprintf("DEV %lu DEV_TX_OFFLOAD_MT_LOCKFREE Yes\n", i);
        else
            dprintf("DEV %lu DEV_TX_OFFLOAD_MT_LOCKFREE No\n", i);
    }
    */
}
//------------------------------------------------------------------------------
// function to set some values
inline void dpdk_init_port_conf(struct rte_eth_conf *conf)
{
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
/*
 * Initialises a given port using global settings and with the rx buffers
 * coming from the mbuf_pool passed as parameter
 */
inline int dpdk_port_init(uint16_t port, struct rte_mempool *mbuf_pool){
	//struct rte_eth_conf port_conf = port_conf_default;
	struct rte_eth_conf port_conf;
    memset(&port_conf, 0, sizeof(struct rte_eth_conf));
    dpdk_init_port_conf(&port_conf);
    dprintf("port_conf.rxmode.max_rx_pkt_len %u 0x%X\n", port_conf.rxmode.max_rx_pkt_len, port_conf.rxmode.max_rx_pkt_len);
	const uint16_t rx_queues = 1, tx_queues = 1;
	int retval;
	uint16_t q;

    dprintf("%s\n", "rte_eth_dev_count()");
	if (port >= rte_eth_dev_count())
		return -1;

    dprintf("%s\n", "rte_eth_dev_configure()");
	retval = rte_eth_dev_configure(port, rx_queues, tx_queues, &port_conf);
	if (retval != 0)
		return retval;

    /*
    retval = rte_eth_dev_set_mtu(port, JUMBO_FRAME_MAX_SIZE);
	if (retval != 0)
		return retval;
    */

    fflush(stdout);
	for (q = 0; q < rx_queues; q++) {
        dprintf("%s %i\n", "rte_eth_rx_queue_setup()", q);
		retval = rte_eth_rx_queue_setup(port, q, RTE_TEST_RX_DESC_DEFAULT,
				                rte_eth_dev_socket_id(port), NULL, mbuf_pool);
		if (retval < 0)
			return retval;
	}

    fflush(stdout);
	for (q = 0; q < tx_queues; q++) {
        dprintf("%s %i\n", "rte_eth_tx_queue_setup()", q);
		retval = rte_eth_tx_queue_setup(port, q, RTE_TEST_TX_DESC_DEFAULT,
				                rte_eth_dev_socket_id(port), NULL);
		if (retval < 0)
			return retval;
	}

	// Ring is written by different access threads and ONLY consumed by the transmit thread
    char name[128];
    sprintf(name, "tx_ring%d", port);
	TX_RING_[port] = rte_ring_create(name, TX_RING_SIZE, rte_eth_dev_socket_id(port), RING_F_SC_DEQ);
	if (TX_RING_[port] == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create output ring\n");

    rte_eth_stats_reset(port);

    dprintf("%s\n", "rte_eth_dev_start()");
	retval  = rte_eth_dev_start(port);
	if (retval < 0)
		return retval;

	struct ether_addr addr;

	rte_eth_macaddr_get(port, &addr);
	printf("Port %u MAC: %02X %02X %02X %02X %02X %02X\n",
			(unsigned)port,
			addr.addr_bytes[0], addr.addr_bytes[1],
			addr.addr_bytes[2], addr.addr_bytes[3],
			addr.addr_bytes[4], addr.addr_bytes[5]);

    dprintf("%s\n", "rte_eth_promiscuous_enable()");
	rte_eth_promiscuous_enable(port);
    dprintf("%s\n", "rte_eth_add_tx_callback()");
	rte_eth_add_tx_callback(port, 0, dpdk_classify_tx, NULL); //TODO turn off for speed
    dprintf("%s\n", "rte_eth_add_rx_callback()");
	rte_eth_add_rx_callback(port, 0, dpdk_classify_rx, NULL); //TODO turn off for speed

	return 0;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// send a query packet on port 'port'
inline void send_query(uint8_t port){
    struct rte_mbuf *m;
    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    TODO("Update with MAC address\n");
    //create_query_packet(m, port);
    printf_packet(m);

    size_t ret = send_burst_fast(port, &m, 1, SEND_BURST_RETRY);
    dprintf("ret %lu\n", ret);
}
//------------------------------------------------------------------------------
uint64_t dpdk_alloc(Location l, size_t size, unsigned char* uuid){
    //PRINTF("dpdk_alloc : %ld bytes @ ", size); location_print(l);
    struct      rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);
    Addr        local_size;
    Addr        local_addr;

    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return 0;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_alloc_packet(m, port, l.mac_addr, size, (Addr)&local_size, (Addr)&local_addr, pak_num, uuid);
    //printf_packet(m);

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkWaitFor(pak_num);

    //PRINTF("ALLOC : slave allocated %ld @ %lx\n", local_size, local_addr);
    return (Addr)((local_size == 0)?0:local_addr);
}
//------------------------------------------------------------------------------
void dpdk_dealloc(Location l, Addr ptr, unsigned char* uuid){
    //PRINTF("dpdk_dealloc : %lx @ %d\n", ptr, l);
    struct rte_mbuf *m;
    uint16_t        port = get_port_from_addr(l.mac_addr);

    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_dealloc_packet(m, port, l.mac_addr, ptr, pak_num, uuid);

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    //dpdkWaitFor(pak_num);
}
//------------------------------------------------------------------------------
uint64_t dpdk_alloc(Location l, size_t size, HASH_TYPE hash){
    //PRINTF("dpdk_alloc : %ld bytes @ %d\n", size, l);
    struct rte_mbuf *m;
    Addr        local_size;
    Addr        local_addr;
    uint16_t    port = get_port_from_addr(l.mac_addr);

    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return 0;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_alloc_hash_packet(
        m, 
        port, 
        l.mac_addr,
        size, 
        HASH_STR(hash), 
        HASH_LENGTH, 
        (Addr)&local_size, 
        (Addr)&local_addr,
        pak_num);
    //printf_packet(m);

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkWaitFor(pak_num);

    //PRINTF("ALLOC : slave allocated %ld @ %lx\n", local_size, local_addr);
    return (Addr)((local_size == 0)?0:local_addr);
}
//------------------------------------------------------------------------------
void dpdk_dealloc(Location l, HASH_TYPE hash){
    //PRINTF("dpdk_dealloc : %s @ %d\n",HASH_STR(hash), l);
    struct rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);

    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_dealloc_hash_packet(m, port, l.mac_addr, HASH_STR(hash), HASH_LENGTH, pak_num);

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    //dpdkWaitFor(pak_num);
}
//------------------------------------------------------------------------------
/*
// read some random values out of the store on port 'port'////wtf
void read_random(uint8_t port, size_t burst_delay){
    dprintf("Core ID %u\n", rte_lcore_id());
    size_t mem_pool_size_bits;
    size_t page_size_bits;
    printf("mem_pool_size_bits page_size_bits > ");
    igr(scanf("%lu %lu", &mem_pool_size_bits, &page_size_bits));
    size_t nb_pages_bits = mem_pool_size_bits - page_size_bits;
    size_t nb_pages = 1UL << nb_pages_bits;
    size_t page_size = 1UL << page_size_bits;
    printf("Number of pages: %lu (in bits %lu)\n", nb_pages, nb_pages_bits);
    printf("Page size: %lu\n", page_size);
    printf("Packet length: %lu\n", PKT_HDR_LEN);
    size_t nb_write_bits;
    size_t burst_size;
    printf("nb_read_bits burst_size > ");
    igr(scanf("%lu %lu", &nb_write_bits, &burst_size));
    size_t nb_write = 1UL << nb_write_bits;
    printf("Number of writes: %lu\n", nb_write);
    printf("Burst size: %lu\n", burst_size);

    if (burst_size > nb_write) {
        printf("burst_size > nb_write. Nothing to do.\n");
        return;
    }
    if (burst_size == 0) {
        printf("burst_size == 0. Nothing to do.\n");
        return;
    }
    if (nb_write == 0) {
        printf("nb_write == 0. Nothing to do.\n");
        return;
    }

#if 0
    stats_t before;
    memcpy(&before, &stats, sizeof(stats_t));
#endif

    struct rte_mbuf* bufs[burst_size];
    size_t i = 0;
    //uint64_t now = rte_rdtsc();
    while (i < nb_write) {
        size_t j;
        for (j=0; j<burst_size; j++, i++) {
            bufs[j] = rte_pktmbuf_alloc(mbuf_pool);
            uint64_t addr = MathExt::rand_width(nb_pages_bits-1) << page_size_bits;
//            create_read_packet(bufs[j], port, addr, addr);
            TODO("READ PACKET\n");
        }
//        size_t ret = send_burst_fast(port, bufs, burst_size, SEND_BURST_RETRY);
        (void)send_burst_wait(port, bufs, burst_size, 50);
        rte_delay_us_block(burst_delay);
    }
    //uint64_t cycles = rte_rdtsc() - now;
#if 0
    size_t delta_total_pkts = stats.total_pkts - before.total_pkts;
    size_t delta_total_actual = stats.total_pkts_actual - before.total_pkts_actual;
    dprintf("cycles %lu\n", cycles);
    dprintf("Total_Pkt %lu Actual %lu\n", delta_total_pkts, delta_total_actual);
    double elapsed = (double)cycles / (double)rte_get_tsc_hz();
    double ops = (double)nb_write / elapsed;
    double bps = (double)(delta_total_actual*PKT_HDR_LEN*8)/elapsed;
    dprintf("N_PKT %lu Elapsed %0.6e seconds\n", nb_write, elapsed);
    dprintf("Ops %0.6e Throughput %0.6e bps\n", ops, bps);
#endif
}
*/
//------------------------------------------------------------------------------
// write random data into l2fwd_message
void randomize_message(void){
    l2fwd_message = (uint8_t *)malloc(l2fwd_page_size);
    size_t i;
    for (i = 0; i < l2fwd_page_size; i++)
        l2fwd_message[i] = rand() & 0xff;
    
    dprintf("%lu bytes randomized\n", l2fwd_page_size);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// write random stuff???
/*
void write_random(uint8_t port, size_t burst_delay){
    dprintf("Core ID %u\n", rte_lcore_id());
    size_t mem_pool_size_bits;
    size_t page_size_bits;
    printf("mem_pool_size_bits page_size_bits > ");
    igr(scanf("%lu %lu", &mem_pool_size_bits, &page_size_bits));
    size_t nb_pages_bits = mem_pool_size_bits - page_size_bits;
    size_t nb_pages = 1UL << nb_pages_bits;
    size_t page_size = 1UL << page_size_bits;
    printf("Number of pages: %lu (in bits %lu)\n", nb_pages, nb_pages_bits);
    printf("Page size: %lu\n", page_size);
    size_t packet_len = PKT_HDR_LEN + page_size;
    printf("Packet length: %lu\n", packet_len);
    size_t nb_write_bits;
    size_t burst_size;
    printf("nb_write_bits burst_size > ");
    igr(scanf("%lu %lu", &nb_write_bits, &burst_size));
    size_t nb_write = 1UL << nb_write_bits;
    printf("Number of writes: %lu\n", nb_write);
    printf("Burst size: %lu\n", burst_size);

    if (burst_size > nb_write) {
        printf("burst_size > nb_write. Nothing to do.\n");
        return;
    }
    if (burst_size == 0) {
        printf("burst_size == 0. Nothing to do.\n");
        return;
    }
    if (nb_write == 0) {
        printf("nb_write == 0. Nothing to do.\n");
        return;
    }

#if 0
    stats_t before;
    memcpy(&before, &stats, sizeof(stats_t));
#endif    

    struct rte_mbuf* bufs[burst_size];
    uint8_t data[page_size];
    size_t i = 0;
    //uint64_t now = rte_rdtsc();
    while (i < nb_write) {
        size_t j;
        for (j=0; j<burst_size; j++, i++) {
            bufs[j] = rte_pktmbuf_alloc(mbuf_pool);
            uint64_t addr = MathExt::rand_width(nb_pages_bits-1) << page_size_bits;
            //create_write_packet(bufs[j], port, addr, addr, data, page_size);
            TODO("Write packet\n");
        }
        (void)send_burst_fast(port, bufs, burst_size, SEND_BURST_RETRY);
        rte_delay_us_block(burst_delay);
    }
    //uint64_t cycles = rte_rdtsc() - now;
#if 0    
    size_t delta_total_pkts = stats.total_pkts - before.total_pkts;
    size_t delta_total_actual = stats.total_pkts_actual - before.total_pkts_actual;
    dprintf("cycles %lu\n", cycles);
    dprintf("Total_Pkt %lu Actual %lu\n", delta_total_pkts, delta_total_actual);
    double elapsed = (double)cycles / (double)rte_get_tsc_hz();
    double ops = (double)nb_write / elapsed;
    double bps = (double)(delta_total_actual*packet_len*8)/elapsed;
    dprintf("N_PKT %lu Elapsed %0.6e seconds\n", nb_write, elapsed);
    dprintf("Ops %0.6e Throughput %0.6e bps\n", ops, bps);
#endif    
}
*/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// write based on ptr
void dpdk_write(
    Location    l, 
    Addr        remote_ptr, 
    size_t      offset, 
    Addr        local_ptr, 
    size_t      size)
{
    struct rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);
    
    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_write_packet(m, 
                        port, 
                        l.mac_addr,
                        remote_ptr, 
                        offset, 
                        local_ptr, 
                        size, 
                        pak_num);
    PRINTF("dpdk_write : [%ld] %d: %lx (%ld bytes) ==> %lx\n", pak_num, port, (Addr)local_ptr, size, remote_ptr);
    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
}
//------------------------------------------------------------------------------
// write based on hash
void dpdk_write_hash(
    Location    l, 
    HASH_TYPE   hash, 
    size_t      offset, 
    Addr        local_addr,
    size_t      size
){
    struct rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);
    
    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_write_hash_packet(m, 
                            port, 
                            l.mac_addr,
                            HASH_STR(hash), 
                            HASH_LENGTH, 
                            offset, 
                            local_addr, 
                            size, 
                            pak_num);

    PRINTF("dpdk_write : [%ld] %d: %lx (%ld bytes) ==> %s\n", pak_num, port, local_addr, size, HASH_STR(hash));

    //std::string tmp_hash;
    //tmp_hash.assign((char*) (rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN) + size), 33);
    //printf("HASH : %s\n", tmp_hash.c_str());

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    //dpdkWaitFor(pak_num);
}
//------------------------------------------------------------------------------
// read based on hash
void dpdk_read(
    Location    l,
    HASH_TYPE   hash, 
    size_t      offset, 
    Addr        local_addr,
    size_t      size
){
    struct rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);
    
    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_read_hash_packet(
        m, 
        port, 
        l.mac_addr,
        HASH_STR(hash), 
        HASH_LENGTH, 
        offset,
        size,
        local_addr,
        pak_num);


    PRINTF("dpdk_read : [%ld] %d: %lx (%ld bytes) ==> %s\n", pak_num, port, local_addr, size, HASH_STR(hash));
    //std::string tmp_hash;
    //tmp_hash.assign((char*) (rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN) + size), 33);
    //printf("HASH : %s\n", tmp_hash.c_str());

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkWaitFor(pak_num);
}
//------------------------------------------------------------------------------
// read based on ptr
void 
dpdk_read(
    Location    l,
    Addr        remote_ptr, 
    size_t      offset, 
    Addr        local_addr,
    size_t      size
){
    struct rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);

    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_read_packet(
        m, 
        port, 
        l.mac_addr,
        remote_ptr,
        offset,
        size,
        local_addr,
        pak_num);

    PRINTF("dpdk_read : [%ld] %d: %lx (%ld bytes) <== %lx\n", pak_num, port, (Addr)local_addr, size, remote_ptr);
    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkWaitFor(pak_num);
}
//------------------------------------------------------------------------------
void
dpdk_get_hash(Location l, HASH_TYPE hash, size_t *alloc_size, char* remote_ptr){
    struct rte_mbuf *m;
    uint16_t    port = get_port_from_addr(l.mac_addr);

    if (unlikely ((m = rte_pktmbuf_alloc(mbuf_pool)) == NULL)) {
        dprintf("%s\n", "ERROR Can not create mbuf for reply"); //TODO better error control
        return;
    }

    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    create_get_hash_packet(
        m, 
        port, 
        l.mac_addr,
        HASH_STR(hash),
        HASH_LENGTH,
        (Addr)alloc_size,
        (Addr)remote_ptr,
        pak_num);

    rte_ring_enqueue(TX_RING_[port], (void*)m);

    // now wait for the packet
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkWaitFor(pak_num);       

    // by this point, the size and pointer values (if any), will be written
}
//------------------------------------------------------------------------------
 //------------------------------------------------------------------------------
// wait for all size packets specified in pak_nums to be received
void dpdkBulkWaitFor(uint64_t *pak_nums, size_t size) {
    bool done = false;
    size_t transport_ids[size];
    for (size_t i = 0; i < size; i++)
        transport_ids[i] = pak_nums[i] % TRANSPORT_WINDOW_SIZE;
    // Wait until all packets received
    while (!done) {
        size_t count = 0;
        for (size_t i = 0; i < size; i++) {
            if (!dpdkIsPacketReceived(transport_ids[i])) {
                if (RETURN_WAIT_US)
                    std::this_thread::sleep_for(std::chrono::microseconds(RETURN_WAIT_US));
                    //rte_delay_us_block(size*RETURN_WAIT_US);
                continue;
            } else {
                count++;
            }
        }
        if (count == size)
            done = true;
    }
    for (size_t i = 0; i < size; i++)
        dpdkSetPacketReceivedFalse(transport_ids[i]);
}
//------------------------------------------------------------------------------
/**
 * RAID0 routing
 */
/*
void dpdkRoutingKVS(uint64_t key, uint64_t &s_addr, uint64_t &d_addr){
    s_addr = key << l2fwd_page_size_bits;
    d_addr = key << (l2fwd_page_size_bits - NUM_SLAVE_NODES_IN_BITS);
}
*/
//------------------------------------------------------------------------------
/** Asynchronous Get
 * \param key A page number in cluster memory space
 * \param value Address to write returned data
 */
/*
uint64_t
dpdkGetAsync(uint64_t key, uint8_t *ret_buffer)
{
    uint64_t addr;
    uint64_t dst;
    dpdkRoutingKVS(key, addr, dst);
    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    for (size_t port_id = 0; port_id < nb_ports; port_id++) {
        tx_pull_v6(port_id, addr, dst, (uint64_t)(ret_buffer+port_id*slave_page_size), pak_num);
    }
    return pak_num;
}
*/
//------------------------------------------------------------------------------
/** Synchronous Get
 * \param key A page number in cluster memory space
 * \param value Address to write returned data
 */
/*
void dpdkGet(uint64_t key, uint8_t *ret_buffer){
    uint64_t pak_num = dpdkGetAsync(key, ret_buffer);
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkWaitFor(pak_num);
}
*/
//------------------------------------------------------------------------------
/** Bulk Synchronous Get
 * \param keys An array of keys
 * \param ref_buffers An array of address to write returned data
 */
/*
void dpdkBulkGet(uint64_t *keys, uint8_t *ret_buffers[], size_t size){
    size_t pak_nums[size];
    for (size_t i = 0; i < size; i++) {
        pak_nums[i] = dpdkGetAsync(keys[i], ret_buffers[i]);
    }
    if (ROUNDTRIP_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(ROUNDTRIP_WAIT_US));
    dpdkBulkWaitFor(pak_nums, size);
}
*/
//------------------------------------------------------------------------------
// some measurement code
/*
void dpdk_time_points(const std::string &prefix){
    std::cout << prefix << "=== " << __func__ << std::endl;
    size_t len = DPDK_time_points.size();
    std::cout << prefix << "Number of time points: " << len << std::endl;
    if (len == 0)
        return;

    std::ofstream out_file("dpdk_time_points.txt");
    if (out_file.is_open()) {
        std::vector< MeanEstimator<double> > means(DPDK_time_points[0].size()-1);
        for (size_t row = 0; row < len; row++) {
            if (DPDK_time_points[row].size() < 2)
                continue;

            out_file << row;
            for (size_t i = 0; i < DPDK_time_points[row].size()-1; i++) {
                std::chrono::duration<double, std::micro> fp_usec = DPDK_time_points[row][i+1].second - DPDK_time_points[row][i].second;
                double dur_usec = fp_usec.count();
                out_file << " X" << DPDK_time_points[row][i+1].first
                        << "-" << DPDK_time_points[row][i].first << "X"
                        << " " << dur_usec;
                means[i].record(dur_usec);
            }
            out_file << "\n";
        }
        std::cout << "mean durations";
        for (auto dur : means) {
            std::cout << " " << dur.estimate();
        }
        std::cout << std::endl;
        out_file.close();
    } else {
        std::cout << prefix << "Could not open dpdk_time_points.txt for writing" << std::endl;
    }

    DPDK_time_points.clear();
}
 */ 
//------------------------------------------------------------------------------
/*
void dpdk_stats(const std::string &prefix){
    std::cout << prefix << "Port 0 rx.waited " << port_statistics[0].rx.waited << std::endl;
    std::cout << prefix << "Port 1 rx.waited " << port_statistics[1].rx.waited << std::endl;

    dpdk_eth_stats_print(0);
    dpdk_eth_stats_print(1);
}
 */ 
//------------------------------------------------------------------------------
/*
void dpdk_time_sizes(const std::string &prefix){
    std::cout << prefix << "=== " << __func__ << std::endl;
    size_t len = DPDK_time_sizes.size();
    std::cout << prefix << "Number of time sizes: " << len << std::endl;

    std::ofstream out_file("dpdk_time_sizes.txt");
    if (out_file.is_open()) {
        for (size_t row = 0; row < len; row++) {
            out_file << row
                     << " " << DPDK_time_sizes[row].second;
            out_file << "\n";
        }
        out_file.close();
    } else {
        std::cout << prefix << "Could not open dpdk_time_sizes.txt for writing" << std::endl;
    }

    DPDK_time_sizes.clear();
}
 */ 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/**
 * \param key A page number in cluster memory space
 * \param data Address to source data
 * \return unique packet number
 */
/*
uint64_t dpdkSetAsync(uint64_t key, uint8_t *data){
    uint64_t addr;
//    size_t port_id;
    uint64_t dst;
    dpdkRoutingKVS(key, addr, dst);
    uint64_t pak_num = packet_number.fetch_add(1, std::memory_order_seq_cst);
    for (size_t port_id = 0; port_id < nb_ports; port_id++) {
        //tx_push_v6(port_id, addr, dst, data+port_id*slave_page_size, pak_num);
    }
    return pak_num;
}
*/
//------------------------------------------------------------------------------
// blocking write of data
/*
uint64_t dpdkSetAsyncWait(uint64_t key, uint8_t *data){
    uint64_t pak_num = dpdkSetAsync(key, data);
    if (SET_ASYNC_WAIT_US)
        std::this_thread::sleep_for(std::chrono::microseconds(SET_ASYNC_WAIT_US));
        //rte_delay_us_block(SET_ASYNC_WAIT_US);
    return pak_num;
}
*/
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// the real application passive packet processor
void dpdk_recv_worker(struct rte_mbuf *m, unsigned port_id) {
    uint8_t*    data;
    uint64_t    packet_number;
    size_t      transport_id;

    struct ether_hdr* recv_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);    

    struct ether_addr my_addr;
    rte_eth_macaddr_get(port_id, &my_addr);
#if 0
    printf("ETH_HDR - SRC : %02X %02X %02X %02X %02X %02X\n", recv_hdr->s_addr.addr_bytes[0], recv_hdr->s_addr.addr_bytes[1], recv_hdr->s_addr.addr_bytes[2], recv_hdr->s_addr.addr_bytes[3], recv_hdr->s_addr.addr_bytes[4], recv_hdr->s_addr.addr_bytes[5]);
    printf("ETH_HDR - DST : %02X %02X %02X %02X %02X %02X\n", recv_hdr->d_addr.addr_bytes[0], recv_hdr->d_addr.addr_bytes[1], recv_hdr->d_addr.addr_bytes[2], recv_hdr->d_addr.addr_bytes[3], recv_hdr->d_addr.addr_bytes[4], recv_hdr->d_addr.addr_bytes[5]);
#endif
    
    if(!compare_ether_hdr(recv_hdr->d_addr, my_addr)){
        //printf("DROP\n");
        rte_pktmbuf_free(m);
        return;
    }

    //printf_packet(m); //TODO debug
	RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
	switch (rum_hdr->type) {
		case UNKNOWN:
			rte_pktmbuf_free(m);
			break;
		case READ:
			rte_pktmbuf_free(m);
            break;
        case READ_HASH:
            rte_pktmbuf_free(m);
            break;
		case WRITE:
			rte_pktmbuf_free(m);
            break;
        case WRITE_HASH:
            rte_pktmbuf_free(m);
            break;
		case QUERY:
			rte_pktmbuf_free(m);
			break;
        case GET:
            rte_pktmbuf_free(m);
            break;
		case DATA:
            // TODO what if packet has bit errors
			data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
			rte_memcpy((void *)(rum_hdr->client_ptr_addr), data, rum_hdr->data_len);
            packet_number   = rum_hdr->pak_num;
            transport_id    = packet_number % TRANSPORT_WINDOW_SIZE;
            packet_received[port_id][transport_id] = true;
            //printf("%d: pak %lu transport %lu\n", port_id, packet_number, transport_id);
            //REQUESTS[port_id].add0erase1(rum_hdr->dst.hi, 0); // packet number
            //size_t ret = REQUESTS[port_id].add0erase1(rum_hdr->src.hi, 1);
            //if (ret != 1)
            //    dprintf("Did not find addr 0x%016lX in request. ret %lu\n", rum_hdr->src.hi, ret);
			rte_pktmbuf_free(m);
			break;
        case GET_REPLY:
            data = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

            /*
            printf("GET_REPLY\n");
            printf("GET_REPLY: REMOTE:: size %ld, ptr %lx\n", rum_hdr->data_len, rum_hdr->slave_ptr_addr);
            printf("GET_REPLY: LOCAL :: size_ptr %lx, local_ptr %lx\n", rum_hdr->client_size_addr, rum_hdr->client_ptr_addr);
            */

            // write the size and remote ptr respectively
            rte_memcpy((void*)rum_hdr->client_size_addr, &rum_hdr->data_len, sizeof(Addr));
            rte_memcpy((void*)rum_hdr->client_ptr_addr, &rum_hdr->slave_ptr_addr, sizeof(Addr));

            // inform of completion
            packet_number   = rum_hdr->pak_num;
            transport_id    = packet_number % TRANSPORT_WINDOW_SIZE;
            packet_received[port_id][transport_id] = true;
            rte_pktmbuf_free(m);
            break;    
		case ANSWER:
            printf_packet(m);
			rte_pktmbuf_free(m);
			break;
        case ALLOC:
            printf_packet(m);
            rte_pktmbuf_free(m);
            break;
        case ALLOC_REPLY:
            //printf_packet(m);
            data        = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);

            /*
            printf("ALLOC_REPLY\n");
            printf("ALLOC_REPLY: REMOTE:: size %ld, ptr %lx\n", rum_hdr->data_len, rum_hdr->slave_ptr_addr);
            printf("ALLOC_REPLY: LOCAL :: size_ptr %lx, local_ptr %lx\n", rum_hdr->client_size_addr, rum_hdr->client_ptr_addr);
            */

            // write the size and remote ptr respectively
            rte_memcpy((void*)rum_hdr->client_size_addr, &rum_hdr->data_len, sizeof(Addr));
            rte_memcpy((void*)rum_hdr->client_ptr_addr, &rum_hdr->slave_ptr_addr, sizeof(Addr));

            // inform of completion
            packet_number   = rum_hdr->pak_num;
            transport_id    = packet_number % TRANSPORT_WINDOW_SIZE;
            packet_received[port_id][transport_id] = true;
            rte_pktmbuf_free(m);
            break;    
        case DEALLOC:
            printf_packet(m);
            rte_pktmbuf_free(m);
            break;    
        case ALLOC_HASH:
            printf_packet(m);
            rte_pktmbuf_free(m);
            break;    
        case ALLOC_HASH_REPLY:
            //printf_packet(m);
            data        = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
            
            /*
            printf("ALLOC_HASH_REPLY\n");
            printf("ALLOC_HASH_REPLY: REMOTE:: size %ld, ptr %lx\n", rum_hdr->data_len, rum_hdr->slave_ptr_addr);
            printf("ALLOC_HASH_REPLY: LOCAL :: size_ptr %lx, local_ptr %lx\n", rum_hdr->client_size_addr, rum_hdr->client_ptr_addr);
            */

           // write the size and remote ptr respectively
            rte_memcpy((void*)rum_hdr->client_size_addr, &rum_hdr->data_len, sizeof(Addr));
            rte_memcpy((void*)rum_hdr->client_ptr_addr, &rum_hdr->slave_ptr_addr, sizeof(Addr));

            // inform of completion
            packet_number   = rum_hdr->pak_num;
            transport_id    = packet_number % TRANSPORT_WINDOW_SIZE;
            packet_received[port_id][transport_id] = true;
            rte_pktmbuf_free(m);
            break;
        case DEALLOC_HASH:
            rte_pktmbuf_free(m);
            break;    
		default:
			rte_pktmbuf_free(m);
			break;
	}
}
//------------------------------------------------------------------------------
// per core thread..... -  not used
int dpdk_lcore_recv(__attribute__((unused)) void *arg){
    dprintf("Running on core %u\n", rte_lcore_id());
    dprintf("BURST_SIZE %u\n", BURST_SIZE);

    struct rte_mbuf *bufs[BURST_SIZE];
	struct rte_mbuf *m;
    size_t i;
    for(;;) {
        size_t port;
        for (port = 0; port < nb_ports; port++) {
            size_t n_pkts = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
            if (n_pkts) {
				for (i = 0; i < (n_pkts < PREFETCH_OFFSET ? n_pkts : PREFETCH_OFFSET); i++)
					rte_prefetch0(rte_pktmbuf_mtod(bufs[i], void *));
                for(i = 0; i < n_pkts; ++i) {
                    m = bufs[i];
					if (i+PREFETCH_OFFSET < n_pkts)
						rte_prefetch0(rte_pktmbuf_mtod(bufs[i+PREFETCH_OFFSET], void *));
                    //rte_prefetch0(rte_pktmbuf_mtod(m, void *));
                    dpdk_recv_worker(m, port);
                }
            }
        }
    }
    printf("DONE received\n");
    return 0;
}
//------------------------------------------------------------------------------
int dpdk_lcore_rx_per_port(void *arg) {
    size_t port = (size_t)arg;
    struct rte_mbuf *bufs[BURST_SIZE];
	struct rte_mbuf *m;
	dprintf("lcore_id %u doing RX for port %lu\n", rte_lcore_id(), port);
    dprintf("BURST_SIZE %u\n", BURST_SIZE);
    size_t i;
    lcore_rx_running[port] = true;
    for(;;) {
        size_t n_pkts = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
        if (n_pkts) {
            for (i = 0; i < (n_pkts < PREFETCH_OFFSET ? n_pkts : PREFETCH_OFFSET); i++)
                rte_prefetch0(rte_pktmbuf_mtod(bufs[i], void *));
            for(i = 0; i < n_pkts; ++i) {
                m = bufs[i];
                if (i+PREFETCH_OFFSET < n_pkts)
                    rte_prefetch0(rte_pktmbuf_mtod(bufs[i+PREFETCH_OFFSET], void *));
                //rte_prefetch0(rte_pktmbuf_mtod(m, void *));
                dpdk_recv_worker(m, port);
            }
        }
    }
    printf("DONE received for port %lu\n", port);
    return 0;
}
//------------------------------------------------------------------------------
// get the stored packets to be transmitted and send
int dpdk_lcore_tx(void *arg) {
    size_t port = (size_t)arg;
    struct rte_ring *in_ring = TX_RING_[port];
	struct rte_mbuf *bufs[BURST_SIZE_TX];
    size_t nb_rx;

	dprintf("lcore_id %u doing TX for port %lu\n", rte_lcore_id(), port);
    dprintf("BURST_SIZE_TX %d\n", BURST_SIZE_TX);
	//while (!quit_signal) {
    lcore_tx_running[port] = true;
	while (1) {
        nb_rx = rte_ring_dequeue_burst(in_ring, (void **)bufs, BURST_SIZE_TX, NULL);
        //HISTOGRAM[port].push(nb_rx);
        if (unlikely(nb_rx == 0))
            continue;

        send_burst_fast(port, bufs, nb_rx, SEND_BURST_RETRY);
        if (LCORE_TX_WAIT_US)
            std::this_thread::sleep_for(std::chrono::microseconds(LCORE_TX_WAIT_US));
            //rte_delay_us_block(LCORE_TX_WAIT_US);
	}
	dprintf("Core %u exiting tx task.\n", rte_lcore_id());
	return 0;
}
//------------------------------------------------------------------------------
size_t dpdk_count_true(const bool *running, size_t size){
    size_t count = 0;
    for (size_t i = 0; i < size; i++) {
        if (running[i])
            count++;
    }
    return count;
}
//------------------------------------------------------------------------------
// wait for the cores..
void dpdk_wait_all_lcore(void) {
    dprintf("nb_ports %u\n", nb_ports);
    bool all_running = false;

    printf("Waiting for all lcore's..");
    while (!all_running) {
        printf(".");
        std::this_thread::sleep_for(std::chrono::seconds(1));
        all_running = (dpdk_count_true(lcore_tx_running, RTE_MAX_ETHPORTS) == nb_ports)
                        && (dpdk_count_true(lcore_rx_running, RTE_MAX_ETHPORTS) == nb_ports);
    }
    printf("\n");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/* Main function, does initialisation and calls the per-lcore functions */
int dpdk_main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: program mem_pool_size_bits page_size_bits\n");
        printf("Need more arguments!\n");
        return -1;
    } else {
        return dpdk_main((size_t)atoi(argv[0]), (size_t)atoi(argv[1]));
    }
}
//------------------------------------------------------------------------------
int dpdk_main(size_t data_pool_bits, size_t page_size_bits){
    printf("DPDK MAIN\n");
    dprintf("%s\n", "Hello");
    dprintf("ETHER_HDR_LEN %u PKT_HDR_LEN %lu\n", ETHER_HDR_LEN, PKT_HDR_LEN);
    dprintf("ETHER_MAX_LEN %u\n", ETHER_MAX_LEN);
    uint8_t portid;

    /* init EAL */
    std::string fake_arg = "dpdk";
    std::vector<char*> argv_list;
    argv_list.push_back((char*)fake_arg.data());
    argv_list.push_back(nullptr);
    int ret = rte_eal_init(argv_list.size()-1, argv_list.data());
    dprintf("rte_eal_init returns %i\n", ret);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    //argc -= ret;
    //argv += ret;

    l2fwd_data_pool_bits = data_pool_bits;
    dprintf("l2fwd_data_pool_bits %lu\n", l2fwd_data_pool_bits);
    l2fwd_page_size_bits = page_size_bits;
    dprintf("l2fwd_page_size_bits %lu\n", l2fwd_page_size_bits);

    dpdk_init_data(0);
    dpdk_init_port_stats();
    dpdk_show_capability();
    dpdk_init_transport();

    rte_pdump_init(NULL);

    nb_ports = rte_eth_dev_count();
    dprintf("rte_eth_dev_count / nb_ports %i\n", nb_ports);
    //nb_ports = 1;
    
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
            NUM_MBUFS * nb_ports - 1, MBUF_CACHE_SIZE, 0,
            8192, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

    /* initialize all ports */
    for (portid = 0; portid < nb_ports; portid++)
		if (dpdk_port_init(portid, mbuf_pool) != 0)
			rte_exit(EXIT_FAILURE, "Cannot init port %d\n", portid);

    dpdk_check_link_status(nb_ports, 0xFF); // nb_ports, enabled_port_mask

    // TODO adjust to number of core used by this implementation
	if (rte_lcore_count() > LCORE_NEEDED)
		printf("\nWARNING: Too much enabled lcores - App uses only %i lcore\n", LCORE_NEEDED);

    // upper layer input queue
    // rte_ring_create

    /*
    unsigned lcore_id = 1;
    for (portid = 0; portid < nb_ports; portid++) {
        rte_eal_remote_launch(dpdk_lcore_rx_per_port, (void *)portid, lcore_id++);
        rte_eal_remote_launch(dpdk_lcore_tx, (void *)portid, lcore_id++);
    }
    */
    unsigned lcore_id;
    RTE_LCORE_FOREACH_SLAVE(lcore_id) {
        if (lcore_id == 1) {
            dprintf("Starting dpdk_lcore_tx port 0 on lcore_id %d\n", lcore_id);
            rte_eal_remote_launch(dpdk_lcore_tx, (void *)0, lcore_id);
        } else if (lcore_id == 2) {
            dprintf("Starting dpdk_lcore_tx port 1 on lcore_id %d\n", lcore_id);
            rte_eal_remote_launch(dpdk_lcore_tx, (void *)1, lcore_id);
        } else if (lcore_id == 3) {
            dprintf("Starting dpdk_lcore_rx_per_port port 0 on lcore_id %d\n", lcore_id);
            rte_eal_remote_launch(dpdk_lcore_rx_per_port, (void *)0, lcore_id);
        } else if (lcore_id == 4) {
            dprintf("Starting dpdk_lcore_rx_per_port port 1 on lcore_id %d\n", lcore_id);
            rte_eal_remote_launch(dpdk_lcore_rx_per_port, (void *)1, lcore_id);
        }
    }

	/* call lcore_main on master core only */
    //dpdk_shell(argc, argv);
    dpdk_wait_all_lcore();

	return 0;
}
//------------------------------------------------------------------------------
int dpdk_end(void){
    printf("%s\n", __func__);
    return rte_pdump_uninit();
}
//------------------------------------------------------------------------------