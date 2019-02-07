#ifndef __LOCATION_H__
#define __LOCATION_H__

/**
 * A Location is used to specify a node at the moment
 */

#define ETHER_ADDR_LEN_BYTES 6
#define IP_ADDR_LEN_BYTES 45

/**
* mac_address : the mac address of dpdk network card, for high speed memory
* ip_address  : the ip address of the UDP hearbeat server for the above slave
*/
typedef struct location{
	unsigned char ip_addr[IP_ADDR_LEN_BYTES];
	unsigned char mac_addr[ETHER_ADDR_LEN_BYTES];
} Location;

const char* location_to_string(Location A);
char* location_to_string_b(Location A, char *buf);

void location_print(Location l);

int location_compare(Location A, Location B);

#endif /*__LOCATION_H__*/