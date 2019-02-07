#include <string.h>

#include "packet.h"

#define DEBUG 1
#include "debug.h"

#if DEBUG
#define PRINTF(...) {printf("\033[1;34mDEBUG:\033[0m "); printf(__VA_ARGS__);}
#define ERROR(...)  {printf("\033[0;31mERROR:\033[0m "); printf(__VA_ARGS__);}
#define TODO(...)   {printf("\033[0;35mTODO:\033[0m "); printf(__VA_ARGS__);}
#else
#define PRINTF(...)
#define ERROR(...)
#define TODO(...)
#endif
//------------------------------------------------------------------------------
inline size_t
send_burst_fast(
    uint8_t port, 
    struct rte_mbuf** bufs, 
    size_t nb_pkts,
    size_t retry
)
{
    size_t pkts = 0;
    size_t cnt = nb_pkts;
    while (cnt && retry) {
        size_t ret = rte_eth_tx_burst(port, 0, bufs, cnt);

        pkts += ret;
        cnt -= ret; // TODO possibly wrap-around
        if (!ret)
            retry--;
    }

    // free packets not send
    if (cnt) dprintf("Trying expired. Packets did not send, freeing %lu\n", cnt);
    cnt = pkts;
    while (unlikely (cnt < nb_pkts))
        rte_pktmbuf_free(bufs[cnt++]);


    return pkts;
}
//------------------------------------------------------------------------------
inline size_t
send_burst_wait(
    uint8_t port, 
    struct rte_mbuf** bufs, 
    size_t nb_pkts,
    size_t retry
)
{
    size_t pkts = 0;
    size_t cnt = nb_pkts;
    while (cnt && retry) {
        if (pkts != 0)
            rte_delay_us_block(1); // No wait on the first while loop
        size_t ret = rte_eth_tx_burst(port, 0, bufs, cnt);

        pkts += ret;
        cnt -= ret; // TODO possibly wrap-around
        if (!ret)
            retry--;
    }

    // free packets not send
    if (cnt) dprintf("Trying expired. Packets did not send, freeing %lu\n", cnt);
    cnt = pkts;
    while (unlikely (cnt < nb_pkts))
        rte_pktmbuf_free(bufs[cnt++]);

    return pkts;
}
//------------------------------------------------------------------------------
inline void
fill_ether_hdr(
    struct rte_mbuf *m,
    uint8_t port,
    uint8_t*        dest_mac
)
{
    struct ether_hdr *e_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);
    

    // note the following order is important as we are probably resuing m

    // set the destination MAC address
    //memset(e_hdr->s_addr.addr_bytes, 0x11, ETHER_ADDR_LEN); // broadcast
    //memset(e_hdr->d_addr.addr_bytes, 0xff, ETHER_ADDR_LEN); // broadcast
    memcpy(e_hdr->d_addr.addr_bytes, dest_mac, ETHER_ADDR_LEN);

    // set the source MAC address
    rte_eth_macaddr_get(port, &e_hdr->s_addr);

#if 0
    //dprintf("e_hdr ptr 0x%08lX\n", (uint64_t)dest_mac);
    unsigned char* a = e_hdr->s_addr.addr_bytes;
    unsigned char* b = e_hdr->d_addr.addr_bytes;
    printf("ETH_HDR - SRC : %02X %02X %02X %02X %02X %02X\n", a[0], a[1], a[2], a[3], a[4], a[5]);
    printf("ETH_HDR - DST : %02X %02X %02X %02X %02X %02X\n", b[0], b[1], b[2], b[3], b[4], b[5]);
#endif

    if (m->pkt_len > ETHER_MAX_LEN)
        e_hdr->ether_type = ETHERTYPE_JUMBO;
    else
        e_hdr->ether_type = m->pkt_len;
}
//------------------------------------------------------------------------------
inline void 
create_write_packet(
    struct rte_mbuf *m,
    uint8_t         port,
    uint8_t*        dest_mac,
    uint64_t        remote_addr,
    size_t          offset,
    uint64_t        local_addr,
    size_t          length,
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN + length;
    m->pkt_len  = PKT_HDR_LEN + length;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type           = WRITE;
    rum_hdr->payload_len    = length;
    rum_hdr->data_len       = length;
    rum_hdr->slave_ptr_addr = remote_addr;
    rum_hdr->client_ptr_addr= local_addr;
    rum_hdr->offset         = offset;
    rum_hdr->pak_num        = pak_num;

    uint8_t *ptr = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
    rte_memcpy(ptr, (char*)local_addr, length);
}
//------------------------------------------------------------------------------
inline void
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
)
{
    m->data_len = PKT_HDR_LEN + (size + hash_len);
    m->pkt_len  = PKT_HDR_LEN + (size + hash_len);

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type           = WRITE_HASH;
    rum_hdr->payload_len    = (size + hash_len);
    //rum_hdr->slave_ptr_addr = remote_addr;
    rum_hdr->data_len       = size;
    rum_hdr->offset         = offset;
    rum_hdr->pak_num        = pak_num;

    uint8_t *ptr = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
    rte_memcpy(ptr, (char*)local_addr, size);
    rte_memcpy(ptr + size, hash, hash_len);
}
//------------------------------------------------------------------------------
inline void
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
){
    m->data_len = PKT_HDR_LEN + (hash_len);
    m->pkt_len  = PKT_HDR_LEN + (hash_len);

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type           = READ_HASH;
    rum_hdr->payload_len    = hash_len;
    rum_hdr->data_len       = size;
    rum_hdr->client_ptr_addr= ret_buffer;
    rum_hdr->offset         = offset;
    rum_hdr->pak_num        = pak_num;

    uint8_t *ptr = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
    rte_memcpy(ptr, hash, hash_len);
}
//------------------------------------------------------------------------------
inline void
create_read_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    uint64_t        remote_ptr,
    size_t          offset, 
    size_t          size,
    uint64_t        ret_buffer,
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN;
    m->pkt_len  = PKT_HDR_LEN;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type               = READ;
    rum_hdr->payload_len        = 0;
    rum_hdr->data_len           = size;
    rum_hdr->slave_ptr_addr     = remote_ptr;    
    rum_hdr->client_ptr_addr    = ret_buffer;
    rum_hdr->offset             = offset;
    rum_hdr->pak_num            = pak_num;
}
//------------------------------------------------------------------------------
inline void
create_data_packet(
    struct rte_mbuf *m,
    uint8_t         port,
    uint8_t*        dest_mac,
    uint8_t         *data,
    size_t          length,
    uint64_t        remote_ptr,
    uint64_t        ret_buffer,
    uint64_t        pak_num
)
{
    m->data_len = PKT_HDR_LEN + length;
    m->pkt_len  = PKT_HDR_LEN + length;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type               = DATA;
    rum_hdr->payload_len        = length;
    rum_hdr->data_len           = length;
    rum_hdr->slave_ptr_addr     = remote_ptr;    
    rum_hdr->client_ptr_addr    = ret_buffer;
    //rum_hdr->offset             = offset;
    rum_hdr->pak_num            = pak_num;

    uint8_t *ptr = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
    rte_memcpy(ptr, data, length);
}
//------------------------------------------------------------------------------
inline void
create_query_packet(
    struct rte_mbuf *m,
    uint8_t port,
    uint8_t*        dest_mac
)
{
    rte_pktmbuf_append(m, PKT_HDR_LEN);
    
    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type           = QUERY;
    rum_hdr->payload_len    = 0;

    TODO("decide what we need this for now...\n");
}
//------------------------------------------------------------------------------
inline void
create_answer_packet(
    struct rte_mbuf *m,
    uint8_t port,
    uint8_t*        dest_mac,
    uint16_t page_size_bits,
    size_t nb_pages
)
{
    m->data_len = PKT_HDR_LEN + ANSWER_PAYLOAD_LENGTH;
    m->pkt_len = PKT_HDR_LEN + ANSWER_PAYLOAD_LENGTH;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type           = ANSWER;
    rum_hdr->payload_len    = ANSWER_PAYLOAD_LENGTH;
    
    TODO("decide what we need this for now...\n");
    //rum_hdr->ctrl.flow_label_lo = page_size_bits;

    uint64_t *ptr = rte_pktmbuf_mtod_offset(m, uint64_t*, PKT_HDR_LEN);
    *ptr = nb_pages;
}
//------------------------------------------------------------------------------
inline void create_alloc_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t*        dest_mac,
    uint64_t        size, 
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num,
    unsigned char   *uuid
){
    m->data_len = PKT_HDR_LEN + strlen((const char*)uuid) + 1;
    m->pkt_len  = PKT_HDR_LEN + strlen((const char*)uuid) + 1;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = ALLOC;
    rum_hdr->payload_len            = strlen((const char*)uuid) + 1;
    rum_hdr->data_len               = size;
    rum_hdr->pak_num                = pak_num;

    //rum_hdr->src.lo = 0;
    rum_hdr->client_ptr_addr    = ret_buffer_remote_ptr;            
    rum_hdr->client_size_addr   = ret_buffer_remote_ptr_size;
    

    char *ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
    rte_memcpy(ptr, uuid, strlen((const char*)uuid)  + 1);
}
//------------------------------------------------------------------------------
// we are using the same memory as the request, so we skip writing a bunch of fields
// should make this more explicit somewhere
inline void create_alloc_reply_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t*        dest_mac,
    uint64_t        cluster_mem_addr,
    size_t          cluster_mem_size,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN;
    m->pkt_len  = PKT_HDR_LEN;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = ALLOC_REPLY;
    rum_hdr->payload_len            = 0;  
    rum_hdr->slave_ptr_addr         = cluster_mem_addr;
    rum_hdr->data_len               = cluster_mem_size;
    rum_hdr->client_ptr_addr        = ret_buffer_remote_ptr;            
    rum_hdr->client_size_addr       = ret_buffer_remote_ptr_size;
    rum_hdr->pak_num                = pak_num;
}
//------------------------------------------------------------------------------
inline void create_dealloc_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t         *dest_mac,
    uint64_t        addr,
    uint64_t        pak_num,
    unsigned char   *uuid
){
    m->data_len = PKT_HDR_LEN + strlen((const char*)uuid) + 1;
    m->pkt_len  = PKT_HDR_LEN + strlen((const char*)uuid) + 1;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = DEALLOC;
    rum_hdr->payload_len            = strlen((const char*)uuid) + 1;    
    rum_hdr->slave_ptr_addr         = addr;
    //rum_hdr->client_buf_addr        = ret_buffer_remote_ptr;            
    //rum_hdr->client_buf_addr        = ret_buffer_remote_ptr_size;
    rum_hdr->pak_num                = pak_num; 

    char *ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
    rte_memcpy(ptr, uuid, strlen((const char*)uuid) + 1);
}
//------------------------------------------------------------------------------
inline void create_alloc_hash_packet(
    struct rte_mbuf *m, 
    uint16_t        port, 
    uint8_t*        dest_mac,
    size_t          size, 
    char*           hash, 
    size_t          hash_len,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN + hash_len;
    m->pkt_len  = PKT_HDR_LEN + hash_len;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = ALLOC_HASH;
    rum_hdr->payload_len            = hash_len;
    rum_hdr->data_len               = size;

    //rum_hdr->src.lo = 0;
    rum_hdr->client_ptr_addr    = ret_buffer_remote_ptr;            
    rum_hdr->client_size_addr   = ret_buffer_remote_ptr_size; 
    rum_hdr->pak_num            = pak_num; 

    char *ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
    rte_memcpy(ptr, hash, hash_len);
}
//------------------------------------------------------------------------------
inline void create_alloc_hash_reply_packet(
    struct rte_mbuf *m,
    uint16_t        port,
    uint8_t*        dest_mac,
    uint64_t        cluster_mem_addr,
    size_t          cluster_mem_size,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN;
    m->pkt_len  = PKT_HDR_LEN;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = ALLOC_HASH_REPLY;
    rum_hdr->payload_len            = 0;
    rum_hdr->data_len               = cluster_mem_size;
    rum_hdr->slave_ptr_addr         = cluster_mem_addr;
    rum_hdr->client_ptr_addr        = ret_buffer_remote_ptr;            
    rum_hdr->client_size_addr       = ret_buffer_remote_ptr_size; 
    rum_hdr->pak_num                = pak_num;
}
//------------------------------------------------------------------------------
inline void create_dealloc_hash_packet(
    struct rte_mbuf *m, 
    uint16_t        port, 
    uint8_t*        dest_mac,
    char*           hash, 
    size_t          hash_len,
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN + hash_len;
    m->pkt_len  = PKT_HDR_LEN + hash_len;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type               = DEALLOC_HASH;
    rum_hdr->payload_len        = hash_len;
    rum_hdr->pak_num            = pak_num; 
    
    char *ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
    rte_memcpy(ptr, hash, hash_len);
}
//------------------------------------------------------------------------------
inline void 
create_get_hash_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    char*           hash, 
    size_t          hash_len,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN + hash_len;
    m->pkt_len  = PKT_HDR_LEN + hash_len;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = GET;
    rum_hdr->payload_len            = hash_len;
    rum_hdr->pak_num                = pak_num; 

    //rum_hdr->src.lo = 0;
    rum_hdr->client_ptr_addr    = ret_buffer_remote_ptr;            
    rum_hdr->client_size_addr   = ret_buffer_remote_ptr_size;      

    char *ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
    rte_memcpy(ptr, hash, hash_len);
}
//------------------------------------------------------------------------------
inline void
create_get_hash_reply_packet(
    struct rte_mbuf *m,
    uint16_t        port, 
    uint8_t*        dest_mac,
    uint64_t        cluster_mem_addr,
    size_t          cluster_mem_size,
    uint64_t        ret_buffer_remote_ptr_size,
    uint64_t        ret_buffer_remote_ptr, 
    uint64_t        pak_num
){
    m->data_len = PKT_HDR_LEN;
    m->pkt_len  = PKT_HDR_LEN;

    fill_ether_hdr(m, port, dest_mac);

    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    rum_hdr->type                   = GET_REPLY;
    rum_hdr->payload_len         = 0;
    rum_hdr->data_len               = cluster_mem_size;
    rum_hdr->slave_ptr_addr             = cluster_mem_addr;
    rum_hdr->client_ptr_addr    = ret_buffer_remote_ptr;            
    rum_hdr->client_size_addr   = ret_buffer_remote_ptr_size;      
    rum_hdr->pak_num                = pak_num; 
}
//------------------------------------------------------------------------------
#ifdef DEBUG
void
printf_packet(struct rte_mbuf *m)
{
    printf("FILL OUT THE OTHER OPTIONS\n");
    printf("== Packet ==============\n");
    printf("data_len %u pkt_len %u\n", m->data_len, m->pkt_len);
    struct ether_hdr* e_hdr = rte_pktmbuf_mtod(m, struct ether_hdr *);    
    printf_ether_hdr(e_hdr);
    RUM_HDR_T *rum_hdr = rte_pktmbuf_mtod_offset(m, RUM_HDR_T*, ETHER_HDR_LEN);
    printf_rum_hdr(rum_hdr);
    
    uint8_t *   uint8_p;
    uint64_t *  uint64_p;
    switch (rum_hdr->type) {
        case READ:
            printf("PacketType READ\n");
            break;
        case WRITE:
            printf("PacketType WRITE\n");
            uint8_p = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
            printf_data(uint8_p);
            break;
        case DATA:
            printf("PacketType DATA\n");
            uint8_p = rte_pktmbuf_mtod_offset(m, uint8_t*, PKT_HDR_LEN);
            printf_data(uint8_p);
            break;
        case QUERY:
            printf("PacketType QUERY\n");
            break;
        case ANSWER:
            printf("PacketType ANSWER\n");
            //printf("PageSizeBits %u\n", rum_hdr->ctrl.flow_label_lo);
            uint64_p = rte_pktmbuf_mtod_offset(m, uint64_t*, PKT_HDR_LEN);
            printf("NumberPages %lu\n", *uint64_p);
            break;
        case ALLOC:
            printf("PacketType ALLOC\n");
            //printf("PageSizeBits %u\n", rum_hdr->ctrl.flow_label_lo);
            //ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
            printf("Alloc size  %lu\n", rum_hdr->data_len);
            printf("Addr        %lx\n", rum_hdr->slave_ptr_addr);
            break;
        case ALLOC_HASH:
            printf("PacketType ALLOC_HASH\n");
            //printf("PageSizeBits %u\n", rum_hdr->ctrl.flow_label_lo);
            //ptr = rte_pktmbuf_mtod_offset(m, char*, PKT_HDR_LEN);
            printf("Alloc size  %lu\n", rum_hdr->data_len);
            printf("Addr        %lx\n", rum_hdr->slave_ptr_addr);
            break;
        default:
            printf("PacketType UKNONWN\n");
            break;
    };
    printf("========== Packet End ==\n");
}
#else
void
printf_packet(struct rte_mbuf *m) {}
#endif
//------------------------------------------------------------------------------
#ifdef DEBUG
void
printf_mac(uint8_t *addr)
{
    printf("MAC: %02X %02X %02X %02X %02X %02X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
}
#else
void
printf_mac(uint8_t *addr) {}
#endif
//------------------------------------------------------------------------------
#ifdef DEBUG
void
printf_ether_hdr(struct ether_hdr *e_hdr)
{
    uint8_t *s = e_hdr->s_addr.addr_bytes;
    uint8_t *d = e_hdr->d_addr.addr_bytes;
    printf("== Ethernet ==\n");
    printf("src MAC %02X %02X %02X %02X %02X %02X\n", s[0], s[1], s[2], s[3], s[4], s[5]);
    printf("dst MAC %02X %02X %02X %02X %02X %02X\n", d[0], d[1], d[2], d[3], d[4], d[5]);
    //printf_mac(e_hdr->s_addr.addr_bytes);
    //printf_mac(e_hdr->d_addr.addr_bytes);
    printf("EtherType 0x%04X %d\n", e_hdr->ether_type, e_hdr->ether_type);
}
#else
void
printf_ether_hdr(struct ether_hdr *e_hdr) {}
#endif
//------------------------------------------------------------------------------
#ifdef DEBUG
void
printf_rum_hdr(RUM_HDR_T *rum_hdr)
{
    printf("== RUM ==\n");
    //printf("src hi lo 0x%016lX 0x%016lX\n", rum_hdr->src.hi, rum_hdr->src.lo);
    //printf("dst hi lo 0x%016lX 0x%016lX\n", rum_hdr->dst.hi, rum_hdr->dst.lo);
    printf("Traffic class       %s\n", PKT_STR(rum_hdr->type));
    printf("payload_len         %ld\n", rum_hdr->payload_len);
    printf("data_len            %ld\n", rum_hdr->data_len);
    printf("offset              %ld\n", rum_hdr->offset);
    printf("slave_ptr_addr      %lx\n", rum_hdr->slave_ptr_addr);
    printf("client_ptr_addr     %lx\n", rum_hdr->client_ptr_addr);
    printf("client_size_addr    %lx\n", rum_hdr->client_size_addr);
    printf("pak_num             %ld\n", rum_hdr->pak_num);
}
#else
void
printf_rum_hdr(RUM_HDR_T *rum_hdr) {}
#endif
//------------------------------------------------------------------------------
#ifdef DEBUG
void
printf_data(uint8_t *d)
{
    printf("== Data ==\n%02X %02X %02X %02X %02X %02X %02X %02X\n", d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7]);
}
#else
void
printf_data(uint8_t *d) {}
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------