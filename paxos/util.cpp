/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "util.h"
#include <string.h>
#include <cstdint>
#include <cstring>

size_t value_marshal(VALUE v, char* buf, size_t len, bool encode){
    size_t pos = 0;
    size_t local_len = strlen(v.c_str());
    
    if(encode){std::memcpy(buf + pos, &local_len, sizeof(size_t));}
    pos += sizeof(size_t);
    
    if(encode){std::memcpy(buf + pos, v.c_str(), local_len);}
    pos += local_len;
    PRINTF("value_marshal: node %s (%ld)\n", v.c_str(), local_len);
    return pos;
}

VALUE value_demarshal(char* buf, size_t len, size_t* ret_pos){
    size_t pos = 0;
    size_t local_node_len;
    char*  local_node;
    
    std::memcpy(&local_node_len, buf + pos, sizeof(size_t));
    pos += sizeof(size_t);
    
    local_node = (char*) malloc(local_node_len);
    std::memset(local_node, '\0', local_node_len);
      
    std::memcpy(local_node, buf + pos, local_node_len);
    pos += local_node_len;
    PRINTF("value_demarshal: VALUE %s (%ld)\n", local_node, local_node_len);
    
    TODO("value_demarshal : check that this is ok in c++ (copy)\n");
    VALUE ret_val(local_node);
    //free(local_node);
    
    *ret_pos = pos;
    PRINTF("value_demarshal: VALUE %s (%ld)\n", ret_val.c_str(), ret_val.length());
    return ret_val;
}