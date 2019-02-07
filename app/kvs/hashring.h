/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   hashring.h
 * Author: paul.harvey
 *
 * Created on 2018/10/18, 9:38
 */

#ifndef HASHRING_H
#define HASHRING_H

#include <map>
#include "Value.h"
#include "base64.h"

#include <memory>

#include <Location.h>           // from RUM

using namespace std;

typedef string KEY;
typedef Location NODE;
typedef shared_ptr<void> VALUE;

typedef map<KEY, NODE>              NODE_MAP;
typedef map<KEY, map<KEY, VALUE>>   DATA_MAP;

#include <string>
#define NODE_TO_STRING(l)           (location_to_string((l)))
#define NODE_COMPARE_EQ(a, b)       (location_compare((a), (b)))
static Location null_location       = {{""},{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
#define NODE_NULL                   (null_location)


#define KEY_TO_STRING(l)           ((l))

class hashring {
public:
    hashring();
    hashring(const hashring& orig);
    virtual ~hashring();
    
    KEY    get_next_node(string node_hash);
    void   put_data(KEY k, VALUE v);
    VALUE  get_data(KEY k);
    void   remove_data(KEY k);
    void   relocate_data(KEY data_key, VALUE data_value, 
                                    KEY current_node, KEY new_node);
    

    void   add_node(NODE n);
    void   remove_node(NODE n);
    NODE   get_next_node_by_key(KEY k);
    NODE   lookup_node_by_key(KEY k);
    KEY    get_node_key(NODE v);
    
    void hash_func(const char* str, size_t len, unsigned char* digest_str);
    
    size_t MD5_STR_LENGTH;
    
    //DEBUG
    void dump_node_ring();
    void dump_per_node_data();
    
private:
    // this is the ring hash table for the key decision of nodes and data
    NODE_MAP node_ring;

    // the hash able for the data
    DATA_MAP data_table;
    
    // This is the replication of a single node instance in the hash ring, not the
    // failure avoidance replication.
    size_t replication_factor = 1;
};

#endif /* HASHRING_H */

