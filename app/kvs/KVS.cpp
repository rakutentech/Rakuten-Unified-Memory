/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   KVS.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/10/17, 14:00
 */

#include "KVS.h"
#include "rum.h"

#include <openssl/md5.h>        // for the MD5 stuff
#include <cstring>              //memcpy
#include <sstream>
#include <fstream>



#define DEBUG 1

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
std::vector<std::string> getNextLineAndSplitIntoTokens(std::istream& str);
/*
{
    std::vector<std::string>   result;
    std::string                line;
    std::getline(str,line);

    std::stringstream          lineStream(line);
    std::string                cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty())
    {
        // If there was a trailing comma then add an empty element.
        result.push_back("");
    }
    return result;
}
*/
//------------------------------------------------------------------------------
KVS::KVS() {
     // initalise RUM
    //rum_init(servers);

    //PRINTF("KVS::KVS\n");
    this->ring              = new hashring();
    this->user_key_to_hash  = new map<KEY, string>();
    this->user_key_to_ptr   = new map<KEY, Addr>();
    this->MD5_STR_LENGTH    = (MD5_DIGEST_LENGTH * 2 + 1);
}
//------------------------------------------------------------------------------
KVS::KVS(std::string machines_csv_file_path) {

    // initalise RUM
    rum_init(machines_csv_file_path.c_str());

    //PRINTF("KVS::KVS\n");
    this->ring              = new hashring();
    this->user_key_to_hash  = new map<KEY, string>();
    this->user_key_to_ptr   = new map<KEY, Addr>();
    this->MD5_STR_LENGTH    = (MD5_DIGEST_LENGTH * 2 + 1);

    // read the machines from a file
    std::ifstream machines_stream(machines_csv_file_path);
    std::vector<std::string> vals;

    vals = getNextLineAndSplitIntoTokens(machines_stream);
    while(vals.size() == 2) {
        printf("%s, %s\n",    vals.at(0).c_str(), vals.at(1).c_str());

        Location tmp_l;
        strcpy((char*)&tmp_l.ip_addr, vals.at(0).c_str());
        sscanf(vals.at(1).c_str(), "%2X:%2X:%2X:%2X:%2X:%2X", 
                            (unsigned int *)&tmp_l.mac_addr[0], 
                            (unsigned int *)&tmp_l.mac_addr[1], 
                            (unsigned int *)&tmp_l.mac_addr[2], 
                            (unsigned int *)&tmp_l.mac_addr[3], 
                            (unsigned int *)&tmp_l.mac_addr[4], 
                            (unsigned int *)&tmp_l.mac_addr[5]);
        
        printf("KVS Register machine: ");location_print(tmp_l);

        ring->add_node(tmp_l);

        vals = getNextLineAndSplitIntoTokens(machines_stream);              
    }
    
    //ring->dump_node_ring();
}
//------------------------------------------------------------------------------
KVS::KVS(const KVS& orig) {
    TODO("KVS_COPY\n");
}
//------------------------------------------------------------------------------
KVS::~KVS() {
    delete this->ring;
    delete this->user_key_to_ptr;
    delete this->user_key_to_hash;
}
//------------------------------------------------------------------------------
//void    KVS::add(KEY k, std::shared_ptr<Value> v){
void    KVS::add(KEY k, Value* v){
     PRINTF("KVS::add %s\n", k.c_str());
    char*                       tmp_ptr;
    size_t                      ptr_size;
    map<KEY, Addr>::iterator    ptr_itter;
    Location                    l;

    //PRINTF("KVS::add %s\n", k.c_str());
    HASH_TYPE h;
    this->ring->hash_func(
        KEY_TO_STRING(k).c_str(), 
        KEY_TO_STRING(k).length(), 
        (unsigned char*)h.hash_str);

    TODO("better mem managment\n");
    string hash_str(HASH_STR(h));

    // If already a value, free it
    if((ptr_itter = this->user_key_to_ptr->find(k)) != this->user_key_to_ptr->end()){
        PRINTF("KVS::add: %s already in the KVS\n", k.c_str());
        rum_free((void*)ptr_itter->second, h);
    }

    //printf("add %s (%d bytes)\n", k.c_str(), v->get_len());
    
    // get the hash of the key
    

    // user key to hash (local)
    this->user_key_to_hash->insert({k, hash_str});

    // how much space should we alloc?
    ptr_size = v->marshal_len();

    // based on the key, choose a node
    l = this->ring->get_next_node_by_key(k);

    // now get the memory
    if((tmp_ptr = (char*)rum_alloc(ptr_size, h, l)) == NULL){
        ERROR("BLOCK_STORE_ADD: malloc fail of %ld bytes\n", ptr_size);
    }

    //PRINTF("KVS::add: marshal the data\n");
    // put the data to the remote
    v->marshal(tmp_ptr);

    ///DEBUG//
    //Value *dem_v = Value::demarshal(tmp_ptr);
    //Value::compare_value(v, dem_v);
    /////////

    //PRINTF("KVS::add: store the data\n");
    // record the key to pointer
    this->user_key_to_ptr->insert({k, (Addr)tmp_ptr});
}
//------------------------------------------------------------------------------
//std::shared_ptr<Value>   KVS::get(KEY k){
Value*   KVS::get(KEY k){
    PRINTF("KVS::get %s\n", k.c_str());
    map<KEY, Addr>::iterator    ptr_itter;
    char* ptr;
    Location l;

    if((ptr_itter = this->user_key_to_ptr->find(k)) != this->user_key_to_ptr->end()){
        //PRINTF("KVS::get: local cache\n");
        return Value::demarshal((char*)ptr_itter->second);
    }
    else{
        //PRINTF("KVS::get: ask remote\n");
        HASH_TYPE h;
        this->ring->hash_func(
            KEY_TO_STRING(k).c_str(), 
            KEY_TO_STRING(k).length(), 
            (unsigned char*)h.hash_str);

        TODO("better mem managment\n");
        string hash_str(HASH_STR(h));

        // based on the key, choose a node
        l = this->ring->get_next_node_by_key(k);

        if((ptr = (char*)rum_get(h, l)) != NULL){
            this->user_key_to_hash->insert({k, hash_str});
            this->user_key_to_ptr->insert({k, (Addr)ptr});
            PRINTF("KVS::get: got the remote ptr, now demarshall\n");
            return Value::demarshal(ptr);
        }
        else{
            PRINTF("KVS::get: the requested key does not exist\n");
        }
    }

    return NULL;
}
//------------------------------------------------------------------------------   
void    KVS::remove(KEY k){
    PRINTF("KVS::remove %s\n", KEY_TO_STRING(k).c_str());
    map<KEY, Addr>::iterator    ptr_itter;
    Location l;

    HASH_TYPE h;
    this->ring->hash_func(
        KEY_TO_STRING(k).c_str(), 
        KEY_TO_STRING(k).length(), 
        (unsigned char*)h.hash_str);

    // we either remove a file that this kvs already knows about, or we are 
    // removing an existing file from somewhere else

    if((ptr_itter = this->user_key_to_ptr->find(k)) != this->user_key_to_ptr->end()){
        this->user_key_to_hash->erase(k);
        this->user_key_to_ptr->erase(k);
        rum_free((char*)ptr_itter->second, h);
    }
    else{
        l = this->ring->get_next_node_by_key(k);
        rum_free(l, h);
    }
}
//------------------------------------------------------------------------------    
bool    KVS::contains(KEY k){
    TODO("KVS::contains\n");
    return false;
}
//------------------------------------------------------------------------------    
size_t  KVS::size(void){
    TODO("KVS::size\n");
    return 0;
}
//------------------------------------------------------------------------------
void KVS::debug_dump_all_data(){
    ring->dump_per_node_data();
}
//------------------------------------------------------------------------------
void KVS::end(){
    rum_end();
}
//------------------------------------------------------------------------------