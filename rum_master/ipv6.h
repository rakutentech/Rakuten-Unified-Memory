#ifndef IPV6_H_
#define IPV6_H_

#include <stdint.h>

#define IP6_HDR_LEN sizeof(ip6_hdr_t)
#define IP6_VERSION 6

// https://en.wikipedia.org/wiki/IPv6_packet
typedef struct ip6_ctrl_s
{
#if __BYTE_ORDER == __BIG_ENDIAN
  uint8_t  version          : 4,
           traffic_class_hi : 4;
  uint8_t  traffic_class_lo : 4,
           flow_label_hi    : 4;
  uint16_t flow_label_lo;
#elif __BYTE_ORDER == __LITTLE_ENDIAN
  uint8_t  traffic_class_hi : 4,
           version          : 4;
  uint8_t  flow_label_hi    : 4,
           traffic_class_lo : 4;
  uint16_t flow_label_lo;
#else
  #error "Fix endianness"
#endif

  uint16_t payload_length;
  uint8_t next_header;
  uint8_t hop_limit;
} ip6_ctrl_t;

typedef struct ip6_addr_s
{
  uint64_t lo;
  uint64_t hi;
} ip6_addr_t;

typedef struct ip6_hdr_s
{
  ip6_ctrl_t ctrl;
  ip6_addr_t src;
  ip6_addr_t dst;
} ip6_hdr_t;


#endif // IPV6_H_
