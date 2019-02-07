/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   slave_paxos_msg.h
 * Author: paul.harvey
 *
 * Created on 2018/11/14, 16:47
 */

#ifndef SLAVE_PAXOS_MSG_H
#define SLAVE_PAXOS_MSG_H


#include <stddef.h>
#include <cstdint>
typedef uint64_t Addr;

#include "../rum_master/packet_type.h"
#include "../rum_master/hash_type.h"

typedef struct paxos_slave_msg{
    pkt_type_t  type;
    union{
        struct{
            size_t      size;
            Addr        addr;
        }alloc;
        
        struct{
            HASH_TYPE   hash;
            size_t      size;
            Addr        addr;
        }alloc_hash;
        
        struct{
            Addr        addr;
        }dealloc;
        
        struct{
            HASH_TYPE   hash;
        }dealloc_hash;
        
        struct{
            Addr        addr;
            size_t      offset;
            size_t      length;
            char*       data;
        }write;
        
        struct{
            HASH_TYPE   hash;
            size_t      offset;
            size_t      length;
            char*       data;
        }write_hash;
    }payload;
}PAXOS_SLAVE_MSG;

class slave_paxos_msg{
public:
    slave_paxos_msg(const slave_paxos_msg& orig);
    virtual ~slave_paxos_msg();
    
    /**
    * Record an allocation action to the backup network
    * @param size      Size of the pointer allocated
    * @param slave_ptr Slave's pointer where the space was allocated
    */
    slave_paxos_msg(size_t size, Addr slave_ptr);
    
    /**
    * Record a hash allocation to the backup network
    * @param size      Size of the pointer allocated
    * @param hash      The hash assocaited with the allocated space
    * @param slave_ptr Slave's pointer where the space was allocated
    */
    slave_paxos_msg(size_t size, HASH_TYPE hash, Addr slave_ptr);
    
    /**
    * Record the deletion of memory to the backup network
    * @param slave_ptr Slave's pointer to memory to be deleted
    */
    slave_paxos_msg(Addr slave_ptr);
    
    /**
    * Record the deletion of the hash allocated memory to the backup network
    * @param hash Hash of the memory to be deleted
    */
    slave_paxos_msg(HASH_TYPE hash);
    
    /**
    * Record data update to the network
    * @param ptr       Pointer to memory to be updated
    * @param offset    Offset into the memory to be written
    * @param length    Length of data to be written
    * @param data      Data to be written to memory
    */
    slave_paxos_msg(Addr ptr, size_t offset, size_t length, char* data);
    
    /**
    * Record data update to the backup network
    * @param hash      Hash ID of the data to be written
    * @param offset    Offset into the memory to be written
    * @param length    Length of data to be written
    * @param data      Data to be written to memory
    */
    slave_paxos_msg(HASH_TYPE hash, size_t offset, size_t length, char* data);
    
    
    size_t      marshal(char* buf, size_t len, bool encode);
    static slave_paxos_msg* demarshal(char* buf, size_t len, size_t* ret_pos);
    
    void dump(void);
private:
    PAXOS_SLAVE_MSG m;
    
    // this is for the demarshall
    slave_paxos_msg();
};

#endif /* SLAVE_PAXOS_MSG_H */

