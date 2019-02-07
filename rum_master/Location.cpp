#include "Location.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <string>

const char* location_to_string(Location A){
    char tmp[33 + IP_ADDR_LEN_BYTES];
    
    sprintf(tmp, "%s, %02X:%02X:%02X:%02X:%02X:%02X", A.ip_addr, 
        A.mac_addr[0], A.mac_addr[1], 
        A.mac_addr[2], A.mac_addr[3], 
        A.mac_addr[4], A.mac_addr[5]);

    std::string ret_val(tmp, 33 + IP_ADDR_LEN_BYTES);
    return ret_val.c_str();
}

char* location_to_string_b(Location A, char *buf){
    sprintf(buf, "%s, %02X:%02X:%02X:%02X:%02X:%02X", A.ip_addr, 
        A.mac_addr[0], A.mac_addr[1], 
        A.mac_addr[2], A.mac_addr[3], 
        A.mac_addr[4], A.mac_addr[5]);
    return buf;
}
        
void location_print(Location l){
	printf("Location        : %s, %02X:%02X:%02X:%02X:%02X:%02X\n", l.ip_addr, 
        l.mac_addr[0], l.mac_addr[1], 
        l.mac_addr[2], l.mac_addr[3], 
        l.mac_addr[4], l.mac_addr[5]);
}

int location_compare(Location A, Location B){
    for(int i = 0 ; i < ETHER_ADDR_LEN_BYTES ; i++){
        if(A.mac_addr[i] != B.mac_addr[i]){
            return false;
        }
    }

    for(int i = 0 ; i < IP_ADDR_LEN_BYTES ; i++){
        if(A.ip_addr[i] != B.ip_addr[i]){
            return false;
        }
    }
    return true;
}

