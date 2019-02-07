/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   slave_paxos_msg.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/11/14, 16:47
 */

#include "slave_paxos_msg.h"

#include <string.h> // memcpy

#include "util.h"
//------------------------------------------------------------------------------
const char* packet_type_debug(pkt_type_t type){
    const char* pkt_type_debug[] = {
        "UNKNOWN", 
        "READ", 
        "READ_HASH", 
        "WRITE", 
        "WRITE_HASH", 
        "QUERY", 
        "GET", "GET_REPLY", 
        "DATA", 
        "ANSWER", 
        "ALLOC", "ALLOC_REPLY",
        "DEALLOC",
        "ALLOC_HASH", "ALLOC_HASH_REPLY",
        "DEALLOC_HASH", 
        "END_PACKET"
    };
    return pkt_type_debug[type];
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(){
    memset((void*)&m, 0, sizeof(PAXOS_SLAVE_MSG));
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(const slave_paxos_msg& orig) {
}
//------------------------------------------------------------------------------
slave_paxos_msg::~slave_paxos_msg() {
}
//------------------------------------------------------------------------------
size_t slave_paxos_msg::marshal(char* buf, size_t len, bool encode){
    size_t pos = 0;
    
    PRINTF("slave_paoxs_msg: marshal %s\n", packet_type_debug(m.type));
    
    if(encode){memcpy(buf + pos, &(m.type), sizeof(pkt_type_t));}
    pos +=  sizeof(pkt_type_t);
    
    switch(m.type){
        case ALLOC:
            if(encode){memcpy(buf + pos, &(m.payload.alloc.size), sizeof(size_t));}
            pos +=  sizeof(size_t);
            if(encode){memcpy(buf + pos, &(m.payload.alloc.addr), sizeof(Addr));}
            pos +=  sizeof(Addr);
            break;
        case ALLOC_HASH:
            if(encode){memcpy(buf + pos, &(m.payload.alloc_hash.hash.hash_str), HASH_LENGTH);}
            pos +=  HASH_LENGTH;
            if(encode){memcpy(buf + pos, &(m.payload.alloc_hash.addr), sizeof(Addr));}
            pos +=  sizeof(Addr);
            if(encode){memcpy(buf + pos, &(m.payload.alloc_hash.size), sizeof(size_t));}
            pos +=  sizeof(size_t);
            break;
        case DEALLOC:
            if(encode){memcpy(buf + pos, &(m.payload.dealloc.addr), sizeof(Addr));}
            pos +=  sizeof(Addr);
            break;
        case DEALLOC_HASH:
            if(encode){memcpy(buf + pos, &(m.payload.dealloc_hash.hash), HASH_LENGTH);}
            pos +=  HASH_LENGTH;
            break;
        case WRITE:
            if(encode){memcpy(buf + pos, &(m.payload.write.addr), sizeof(Addr));}
            pos +=  sizeof(Addr);
            if(encode){memcpy(buf + pos, &(m.payload.write.length), sizeof(size_t));}
            pos +=  sizeof(size_t);
            if(encode){memcpy(buf + pos, &(m.payload.write.offset), sizeof(size_t));}
            pos +=  sizeof(size_t);
            if(encode){memcpy(buf + pos, m.payload.write.data, m.payload.write.length);}
            pos += m.payload.write.length;
            break;
        case WRITE_HASH:
            if(encode){memcpy(buf + pos, &(m.payload.write_hash.hash), HASH_LENGTH);}
            pos +=  HASH_LENGTH;
            if(encode){memcpy(buf + pos, &(m.payload.write_hash.length), sizeof(size_t));}
            pos +=  sizeof(size_t);
            if(encode){memcpy(buf + pos, &(m.payload.write_hash.offset), sizeof(size_t));}
            pos +=  sizeof(size_t);
            if(encode){memcpy(buf + pos, m.payload.write_hash.data, m.payload.write_hash.length);}
            pos += m.payload.write_hash.length;
            break;
        default:
            TODO("slave_paxos_msg::marshal\n");
    }
    return pos;
}
//------------------------------------------------------------------------------
slave_paxos_msg* slave_paxos_msg::demarshal(char* buf, size_t len, size_t* ret_pos){
    size_t pos = 0;
    slave_paxos_msg *new_guy = new slave_paxos_msg();
    
    memcpy(&(new_guy->m.type), buf + pos, sizeof(pkt_type_t));
    pos +=  sizeof(pkt_type_t);
    
    switch(new_guy->m.type){
        case ALLOC:
            memcpy(&(new_guy->m.payload.alloc.size), buf + pos, sizeof(size_t));
            pos +=  sizeof(size_t);
            memcpy(&(new_guy->m.payload.alloc.addr), buf + pos, sizeof(Addr));
            pos +=  sizeof(Addr);
            break;
        case ALLOC_HASH:
            memcpy(new_guy->m.payload.alloc_hash.hash.hash_str, buf + pos, HASH_LENGTH);
            pos +=  HASH_LENGTH;
            memcpy(&(new_guy->m.payload.alloc_hash.addr), buf + pos, sizeof(Addr));
            pos +=  sizeof(Addr);
            memcpy(&(new_guy->m.payload.alloc_hash.size), buf + pos, sizeof(size_t));
            pos +=  sizeof(size_t);
            break;
        case DEALLOC:
            memcpy(&(new_guy->m.payload.dealloc.addr), buf + pos, sizeof(Addr));
            pos +=  sizeof(Addr);
            break;
        case DEALLOC_HASH:
            memcpy(new_guy->m.payload.dealloc_hash.hash.hash_str, buf + pos, HASH_LENGTH);
            pos +=  HASH_LENGTH;
            break;
        case WRITE:
            memcpy(&(new_guy->m.payload.write.addr), buf + pos, sizeof(Addr));
            pos +=  sizeof(Addr);
            memcpy(&(new_guy->m.payload.write.length), buf + pos, sizeof(size_t));
            pos +=  sizeof(size_t);
            memcpy(&(new_guy->m.payload.write.offset), buf + pos, sizeof(size_t));
            pos +=  sizeof(size_t);
            
            if((new_guy->m.payload.write.data = (char*)malloc(new_guy->m.payload.write.length)) == NULL){
                ERROR("slave_paxos_msg::demarshal: malloc failed (%ld bytes)\n", new_guy->m.payload.write.length);
            }
            memcpy(new_guy->m.payload.write.data, buf + pos, new_guy->m.payload.write.length);
            pos += new_guy->m.payload.write.length;
            break;
        case WRITE_HASH:
            memcpy(new_guy->m.payload.write_hash.hash.hash_str, buf + pos, HASH_LENGTH);
            pos +=  HASH_LENGTH;
            memcpy(&(new_guy->m.payload.write_hash.length), buf + pos, sizeof(size_t));
            pos +=  sizeof(size_t);
            memcpy(&(new_guy->m.payload.write_hash.offset), buf + pos, sizeof(size_t));
            pos +=  sizeof(size_t);
            
            if((new_guy->m.payload.write_hash.data = (char*)malloc(new_guy->m.payload.write_hash.length)) == NULL){
                ERROR("slave_paxos_msg::demarshal: malloc failed (%ld bytes)\n", new_guy->m.payload.write_hash.length);
            }
            memcpy(new_guy->m.payload.write_hash.data, buf + pos, new_guy->m.payload.write_hash.length);
            pos += new_guy->m.payload.write_hash.length;
            break;
        default:
            TODO("slave_paxos_msg::marshal\n");
    }
    return new_guy;
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(size_t size, Addr slave_ptr){
    m.type = ALLOC;
    m.payload.alloc.addr = slave_ptr;
    m.payload.alloc.size = size;
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(size_t size, HASH_TYPE hash, Addr slave_ptr){
    m.type = ALLOC_HASH;
    m.payload.alloc_hash.hash = hash;
    m.payload.alloc_hash.addr = slave_ptr;
    m.payload.alloc_hash.size = size;
    
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(Addr slave_ptr){
    m.type = DEALLOC;
    m.payload.dealloc.addr = slave_ptr;

}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(HASH_TYPE hash){
    m.type = DEALLOC_HASH;
    m.payload.dealloc_hash.hash = hash;
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(Addr ptr, size_t offset, size_t length, char* data){
    m.type = WRITE;
    m.payload.write.addr    = ptr;
    m.payload.write.data    = data;
    m.payload.write.length  = length;
    m.payload.write.offset  = offset;
}
//------------------------------------------------------------------------------
slave_paxos_msg::slave_paxos_msg(HASH_TYPE hash, size_t offset, size_t length, char* data){
    m.type = WRITE_HASH;
    m.payload.write_hash.data   = data;
    m.payload.write_hash.hash   = hash;
    m.payload.write_hash.length = length;
    m.payload.write_hash.offset = offset;
}
//------------------------------------------------------------------------------
void slave_paxos_msg::dump(void){
    printf("---------------------------------------------\n");
    printf("slave_paxos_msg::dump\n");
    printf("type - %s\n", packet_type_debug(m.type));
    switch(m.type){
        case ALLOC:
            printf("\tsize: %ld, addr: %lx\n", m.payload.alloc.size, m.payload.alloc.addr);
            break;
        case ALLOC_HASH:
            printf("\t hash: %s, addr: %lx, size: %ld\n", m.payload.alloc_hash.hash.hash_str, m.payload.alloc_hash.addr, m.payload.alloc_hash.size);
            break;
        case DEALLOC:
            printf("\tadd: %lx\n", m.payload.dealloc.addr);
            break;
        case DEALLOC_HASH:
            printf("\thash: %s\n", m.payload.dealloc_hash.hash.hash_str);
            break;
        case WRITE:
            printf("\taddr: %lx, length: %ld, offset: %ld\n", m.payload.write.addr, m.payload.write.length, m.payload.write.offset);
            break;
        case WRITE_HASH:
            printf("\thash: %s, length: %ld, offset: %ld\n", m.payload.write_hash.hash.hash_str, m.payload.write_hash.length, m.payload.write_hash.offset);
            break;
        default:
            TODO("slave_paxos_msg::marshal\n");
    }
    printf("---------------------------------------------\n");
}