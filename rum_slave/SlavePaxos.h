/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SlavePaxos.h
 * Author: paul.harvey
 *
 * Created on 2018/11/13, 15:34
 */

#ifndef SLAVEPAXOS_H
#define SLAVEPAXOS_H

#include "../rum_master/hash_type.h"
#include "../paxos/node.h"

#include "slave_paxos_msg.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <vector>
#include <memory>

typedef uint64_t Addr;

class SlavePaxos {
public:
    SlavePaxos(std::vector<std::shared_ptr<Node>> slave_s);
    SlavePaxos(const SlavePaxos& orig);
    virtual ~SlavePaxos();
    
    /**
    * Record an allocation action to the backup network
    * @param size      Size of the pointer allocated
    * @param slave_ptr Slave's pointer where the space was allocated
    */
    void alloc(size_t size, Addr slave_ptr);
    
    /**
    * Record a hash allocation to the backup network
    * @param size      Size of the pointer allocated
    * @param hash      The hash assocaited with the allocated space
    * @param slave_ptr Slave's pointer where the space was allocated
    */
    void alloc_hash(size_t size, HASH_TYPE hash, Addr slave_ptr);
    
    /**
    * Record the deletion of memory to the backup network
    * @param slave_ptr Slave's pointer to memory to be deleted
    */
    void dealloc(Addr slave_ptr);
    
    /**
    * Record the deletion of the hash allocated memory to the backup network
    * @param hash Hash of the memory to be deleted
    */
    void dealloc_hash(HASH_TYPE hash);
    
    /**
    * Record data update to the network
    * @param ptr       Pointer to memory to be updated
    * @param offset    Offset into the memory to be written
    * @param length    Length of data to be written
    * @param data      Data to be written to memory
    */
    void write(Addr ptr, size_t offset, size_t length, char* data);
    
    /**
    * Record data update to the backup network
    * @param hash      Hash ID of the data to be written
    * @param offset    Offset into the memory to be written
    * @param length    Length of data to be written
    * @param data      Data to be written to memory
    */
    void write_hash(HASH_TYPE hash, size_t offset, size_t length, char* data);
private:
    std::vector<std::shared_ptr<Node>> slaves;
    void ship_and_send(slave_paxos_msg *real_msg);
};



#endif /* SLAVEPAXOS_H */

