/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   hashring.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/10/18, 9:38
 */

#include "hashring.h"

#include <openssl/md5.h>
#include <iostream>
#include <list>
#include <string.h>
/*----------------------------------------------------------------------------*/
#define DEBUG 1

#if DEBUG
#define PRINTF(...) {printf("\033[1;34mDEBUG:\033[0m "); printf(__VA_ARGS__);}
#define ERROR(...)	{printf("\033[0;31mERROR:\033[0m "); printf(__VA_ARGS__);}
#define TODO(...)	{printf("\033[0;35mTODO:\033[0m "); printf(__VA_ARGS__);}
#else
#define PRINTF(...)
#define ERROR(...)
#define TODO(...)
#endif
//------------------------------------------------------------------------------
// this buffer assumes serialised access....
static char loc_buf[1024];
#define NODE_TO_STRING(l)           (location_to_string_b((l), loc_buf))
//------------------------------------------------------------------------------
#if DEBUG
void hashring::dump_node_ring(){
    PRINTF("===============================================================\n");
    PRINTF("node_ring: size %ld\n", node_ring.size());
    for (NODE_MAP::iterator it=node_ring.begin(); it!=node_ring.end(); ++it){
        std::cout << "\t\t"  << it->first << " => "; 
        location_print(it->second);
        std::cout << '\n';
    }
    PRINTF("===============================================================\n");
}

#include <algorithm>
void hashring::dump_per_node_data(){
    PRINTF("===============================================================\n");
    //std::list<KEY> keys;

    // for each node in the data table
    for(auto node_iter = data_table.begin() ; node_iter != data_table.end() ; node_iter++){
        PRINTF("Node %s (%s): #data %ld\n", 
            NODE_TO_STRING(this->lookup_node_by_key(node_iter->first)), 
            node_iter->first.c_str(), 
            node_iter->second.size()
        );
        // for each <k,v> pair in the data table
        for(auto data_iter = node_iter->second.begin() ; data_iter != node_iter->second.end() ; data_iter++){
            PRINTF("\t\t %s :: %s\n", data_iter->first.c_str(), "<insert data>");// data_iter->second.c_str());

            //keys.push_back(data_iter->first);
        }
    }

    //keys.sort();
    //for(auto i : keys)
    //	std::cout << i << std::endl;
    PRINTF("===============================================================\n");
}
#endif
//------------------------------------------------------------------------------

hashring::hashring() {
     MD5_STR_LENGTH = (MD5_DIGEST_LENGTH * 2 + 1);
}
//------------------------------------------------------------------------------
hashring::hashring(const hashring& orig) {
}
//------------------------------------------------------------------------------
hashring::~hashring() {
}
//------------------------------------------------------------------------------
KEY     hashring::get_next_node(string node_hash){
    NODE_MAP::iterator it = node_ring.lower_bound(node_hash);
    if(it == node_ring.end()){
        // we are i a ring, so we wrap
        it = node_ring.begin();
    }

    PRINTF("get_next_node: %s :: %s\n", node_hash.c_str(), it->first.c_str());//, it->second.c_str());
    return it->first;
}
//------------------------------------------------------------------------------
void    hashring::relocate_data(KEY data_key, VALUE data_value, KEY current_node, KEY new_node){
    PRINTF("RELOCATE: %s (%s) @ %s\n", data_key.c_str(), "", current_node.c_str());
    try{
        data_table[current_node].erase(data_key);
    }
    catch(std::out_of_range& e){
        PRINTF("RELOCATE: the key was not found\n");
    }

    this->put_data(data_key, data_value);

    TODO("RELOCATE: update the network\n");
}
//------------------------------------------------------------------------------
void    hashring::put_data(KEY k, VALUE v){
    // lookup the appropriate node for the data's key
    NODE n = this->get_next_node_by_key(k);

    // get this nodes key
    KEY node_key = this->get_node_key(n);

    // this is where we contact the other machines
    // but for now we do this locally
    data_table[node_key].insert({k,v});

    PRINTF("PUT_DATA: storing '%s @ %s (%s)\n", k.c_str(), NODE_TO_STRING(n), node_key.c_str());
}
//------------------------------------------------------------------------------
VALUE   hashring::get_data(KEY k){
    DATA_MAP::iterator node_iter;
    std::map<KEY, VALUE>::iterator val_iter;

    // lookup the appropriate node for the data's key
    NODE n = this->get_next_node_by_key(k);

    // get this nodes key
    KEY node_key = this->get_node_key(n);

    // for the given node, find the key
    // get the node's map
    if((node_iter = data_table.find(node_key)) != data_table.end()){
        if((val_iter = node_iter->second.find(k)) != node_iter->second.end()){
            return val_iter->second;
        } 
        ERROR("GET_DATA: node '%s' has no key '%s'\n", NODE_TO_STRING(n), k.c_str());
    }
    ERROR("GET_DATA: can not find node '%s' \n", NODE_TO_STRING(n));
    return nullptr;
}
//------------------------------------------------------------------------------
void    hashring::remove_data(KEY k){
    PRINTF("REMOVE_DATA: removing key %s\n", k.c_str());
    NODE n = this->get_next_node_by_key(k);

    // get this nodes key
    KEY node_key = this->get_node_key(n);

    // get the data
    VALUE v = this->get_data(k);

    // this is where we contact the other machines
    // but for now we do this locally
    try{
        data_table[node_key].erase(k);
    }
    catch(std::out_of_range& e){
            PRINTF("REMOVE_DATA: the key was not found\n");
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void    hashring::add_node(NODE n){
    uint64_t i;
    unsigned char hash[MD5_STR_LENGTH];

    DATA_MAP::iterator node_iter;
    std::map<KEY, VALUE>::iterator val_iter;

    hash_func(NODE_TO_STRING(n), strlen(NODE_TO_STRING(n)), (unsigned char*)&hash[0]);

    for(i = 0 ; i < replication_factor ; i++){
        // add a suffix for each replication
        std::string tmp = NODE_TO_STRING(n);
        tmp += ("_" + std::to_string(i));
        hash_func(tmp.c_str(), tmp.length(), (unsigned char*)&hash[0]);

        KEY new_node_key((char*)hash);

        PRINTF("ADD_NODE: %s (%s)\n", NODE_TO_STRING(n), new_node_key.c_str());

        node_ring.insert(std::make_pair(new_node_key, n));
        PRINTF("ADD_NODE: adding {%s, %s}\n", hash, tmp.c_str());
        
        // now look ahead to the next node. 
        KEY new_node_successor = this->get_next_node(new_node_key);
        PRINTF("ADD_NODE: node to steal data from %s (%s)\n", new_node_successor.c_str(), new_node_key.c_str());

        // if there is data in the successor node and the successor is not us
        if((node_iter = data_table.find(new_node_successor)) != data_table.end() && node_iter->first.compare(new_node_key) != 0){
            PRINTF("ADD_NODE: adding node has potential data to steal from node %s\n", new_node_successor.c_str());

            // for each data held by the node
            for (auto data_iter = node_iter->second.begin(), next_it = node_iter->second.begin(); data_iter != node_iter->second.cend(); data_iter = next_it){
                PRINTF("ADD_NODE: compare %s :: %s \n", data_iter->first.c_str(), new_node_key.c_str());
                next_it = data_iter; ++next_it;

                std::string data_hash = data_iter->first;

                // if we wrap by node hash
                if(new_node_successor < new_node_key){
                    if(data_hash < new_node_key && data_hash > new_node_successor){
                        this->relocate_data(data_iter->first, data_iter->second, new_node_successor, new_node_key);
                    }
                }
                else{
                    // have normal data
                    if(data_hash < new_node_key){
                        this->relocate_data(data_iter->first, data_iter->second, new_node_successor, new_node_key);	
                    }
                    // have data which wrapped
                    else if(data_hash > new_node_successor && new_node_key < new_node_successor){
                        this->relocate_data(data_iter->first, data_iter->second, new_node_successor, new_node_key);
                    }
                }
            }
        }
#if DEBUG
        else {
            PRINTF("ADD_NODE: could not find node to replace\n");
        }
#endif			
    }
    PRINTF("=====\n");
}
//------------------------------------------------------------------------------
void    hashring::remove_node(NODE n){
    uint64_t i;
    char hash[MD5_STR_LENGTH];

    PRINTF("REMOVE_NODE: removing %s\n", NODE_TO_STRING(n));

    for(i = 0 ; i < replication_factor ; i++){
        std::string tmp = NODE_TO_STRING(n);
        tmp += ("_" + std::to_string(i));
        hash_func(tmp.c_str(), tmp.length(), (unsigned char*) &hash[0]);

        KEY node_key((char*)hash);

        // remove from the ring, so it won't be found
        node_ring.erase(node_key);

        DATA_MAP::iterator it = data_table.find(node_key);
        if(it != data_table.end() && !data_table[node_key].empty()){
            PRINTF("REMOVE_NODE: has data\n");

            // so now we need to move the data
            // for each data
            for (auto data_iter = data_table[node_key].begin(), next_it = data_table[node_key].begin(); data_iter != data_table[node_key].cend(); data_iter = next_it){

                next_it = data_iter; ++next_it;

                KEY 	k = data_iter->first;
                VALUE  	v = data_iter->second;

                if(node_ring.size() == 0){
                        TODO("what do we do when delteting the last node\n");
                }
                else{
                        this->put_data(k, v);
                }

                data_table[node_key].erase(k);
            }
        }
        data_table.erase(node_key);
        PRINTF("REMOVE_NODE: removing {%s %s}\n", node_key.c_str(), tmp.c_str());
    }
}
//------------------------------------------------------------------------------
NODE   hashring::get_next_node_by_key(KEY k){
    NODE_MAP::iterator it;
    //char hash[MD5_STR_LENGTH];

    PRINTF("GET_NODE_NEXT_BY_KEY: %s\n", k.c_str());

    if(node_ring.empty()){
        // do something bad
        TODO("GET_NODE_NEXT_BY_KEY: the ring is empty!!\n");
    }

    // first we hash the data
    //hash_func(k.c_str(), (unsigned char*)&hash[0]);

    //std::string hash_val = std::string(hash);

    // based on this hash, look for a server on the ring

    //it = node_ring.lower_bound(std::string(hash));
    it = node_ring.lower_bound(k);
    if(it == node_ring.end()){
        // we are i a ring, so we wrap
        it = node_ring.begin();
    }

    PRINTF("GET_NODE_NEXT_BY_KEY: %s => %s\n", it->first.c_str(), NODE_TO_STRING(it->second));

    return it->second;
}
//------------------------------------------------------------------------------
NODE    hashring::lookup_node_by_key(KEY k){
    //PRINTF("hashring_lookup_node_by_key: %s\n", k.c_str());
    NODE_MAP::iterator it = node_ring.find(k);

    if(it == node_ring.end()){
        ERROR("LOOKUP_NODE_BY_KEY : could not find node for key %s\n", k.c_str());
        dump_node_ring();
        return NODE_NULL;
    }

    return it->second;
}
//------------------------------------------------------------------------------
KEY     hashring::get_node_key(NODE n){
    for(NODE_MAP::iterator node_iter = node_ring.begin() ; node_iter != node_ring.end() ; node_iter++){
        //PRINTF("hashring_get_node_key: copmare %s :: %s\n", node_iter->second.c_str(), v.c_str());
        if(NODE_COMPARE_EQ(node_iter->second, n)){
            return node_iter->first;
        }
    }

    ERROR("Could not find the key for node %s\n", NODE_TO_STRING(n));
    return "";
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// this is based on the openssl library
// http://www.askyb.com/cpp/openssl-md5-hashing-example-in-cpp/
// will require the -lcrypto
void hashring::hash_func(const char* str, size_t len, unsigned char* digest_str){
    static const char hexDigits[17] = "0123456789ABCDEF";
    unsigned char digest[MD5_DIGEST_LENGTH];
    //PRINTF("HASH: %s (len %ld)\n", str, strlen(str));

    MD5( (const unsigned char*) str, len, digest);

    // assume that the digest_str is 33 bytes long
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++){
        digest_str[i*2] 	= hexDigits[(digest[i] >> 4) & 0xF];
        digest_str[(i*2) + 1] 	= hexDigits[digest[i] & 0xF];
    }
    digest_str[MD5_DIGEST_LENGTH * 2] = '\0';

#if 0
    char mdString[33];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);

    //PRINTF("md5 digest: %s\n", mdString);
    PRINTF("md5 digest_str: %s\n", digest_str);
#endif
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------