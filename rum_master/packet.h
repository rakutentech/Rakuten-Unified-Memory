/**
 * Packet related routines
 * 
 * For documentation see the matching source file.
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

#include <rte_ethdev.h>
#include <rte_cycles.h>

#include "packet_type.h"    
#include "ipv6.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO - paul: a packet type should really be a union, and the last field is an
//              array, where the size is determined by the malloc which is made.    



// TODO: replace ipv6 header with RUM_HDR - more compact
//       - this should be a union
typedef struct rum_hdr_t{
    pkt_type_t  type;                   // type of RUM PKT
    size_t      payload_len;            // Length of packet payload
    size_t      data_len;               // Length of data used in the action
    size_t      offset;                 // Offset into the data region
    uint64_t    slave_ptr_addr;         // Slave ptr address
    uint64_t    client_ptr_addr;        // Client ptr address
    uint64_t    client_size_addr;       // Client addr for the size buffer
    uint64_t    pak_num;                // Client seq number of this packet
} RUM_HDR_T;

#define RUM_HDR_LEN (sizeof(struct rum_hdr_t))

#define PKT_HDR_LEN (ETHER_HDR_LEN + RUM_HDR_LEN)
#define ETHERTYPE_JUMBO 0x8870
#define ANSWER_PAYLOAD_LENGTH 8
#define ALLOC_PAYLOAD_LENGTH  (sizeof(size_t) + sizeof(uint64_t))
//------------------------------------------------------------------------------
size_t
send_burst_fast(
    uint8_t port, 
    struct rte_mbuf** bufs, 
    size_t nb_pkts,
    size_t retry
);
//------------------------------------------------------------------------------
size_t
send_burst_wait(
    uint8_t port, 
    struct rte_mbuf** bufs, 
    size_t nb_pkts,
    size_t retry
);
//------------------------------------------------------------------------------
void
fill_ether_hdr(
    struct rte_mbuf *m,
    uint8_t port,
    uint8_t*        dest_mac
);
//------------------------------------------------------------------------------
void
create_write_packet(
    struct rte_mbuf *m,
    uint8_t         port,
    uint8_t*        dest_mac,
    uint64_t        remote_addr,
    size_t          offset,
    uint64_t        local_addr,
    size_t          length,
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void
create_write_hash_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    char*           hash, 
    size_t          hash_len,
    size_t          offset, 
    uint64_t        local_addr,
    size_t          size,
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void
create_read_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    uint64_t        remote_ptr,
    size_t          offset, 
    size_t          size,
    uint64_t        ret_buffer,
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void
create_read_hash_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    char*           hash, 
    size_t          hash_len,
    size_t          offset, 
    size_t          size,
    uint64_t        ret_buffer,
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
/**
 * Ask cluster memory for existing allocation based on the supplied hash
 */
void
create_get_hash_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    char*           hash, 
    size_t          hash_len,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void
create_get_hash_reply_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    uint64_t        cluster_mem_addr,
    size_t          cluster_mem_size,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void
create_data_packet(
    struct rte_mbuf *m,
    uint8_t         port,
    uint8_t*        dest_mac,
    uint8_t         *data,
    size_t          length,
    uint64_t        remote_ptr,
    uint64_t        ret_buffer,
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void
create_query_packet(
    struct rte_mbuf *m,
    uint8_t         port,
    uint8_t*        dest_mac
);
//------------------------------------------------------------------------------
void
create_answer_packet(
    struct rte_mbuf *m,
    uint8_t port,
    uint8_t*        dest_mac,
    uint16_t page_size_bits,
    size_t nb_pages
);
//------------------------------------------------------------------------------
void create_alloc_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t*        dest_mac,
    uint64_t        size, 
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num,
    unsigned char   *uuid
);
//------------------------------------------------------------------------------
void create_alloc_reply_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t*        dest_mac,
    uint64_t        cluster_mem_addr,
    size_t          cluster_mem_size,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void create_dealloc_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t         *dest_mac,
    uint64_t        addr, 
    uint64_t        pak_num,
    unsigned char   *uuid
);
//------------------------------------------------------------------------------
void create_alloc_hash_packet(
    struct rte_mbuf *m, 
    uint16_t        port, 
    uint8_t*        dest_mac,
    size_t          size, 
    char*           hash, 
    size_t          hash_len,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void create_alloc_hash_reply_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t*        dest_mac,
    uint64_t        cluster_mem_addr,
    size_t          cluster_mem_size,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void create_dealloc_hash_packet(
    struct rte_mbuf *m, 
    uint16_t        port, 
    uint8_t*        dest_mac,
    char*           hash, 
    size_t          hash_len,
    uint64_t        pak_num
);
//------------------------------------------------------------------------------
void printf_packet(struct rte_mbuf *m);
//------------------------------------------------------------------------------
void printf_mac(uint8_t *addr);
//------------------------------------------------------------------------------
void printf_ether_hdr(struct ether_hdr *e_hdr);
//------------------------------------------------------------------------------
void printf_rum_hdr(RUM_HDR_T *rum_hdr);
//------------------------------------------------------------------------------
void printf_data(uint8_t *d);
//------------------------------------------------------------------------------
#ifdef __cplusplus
}
#endif

#endif // PACKET_H_
