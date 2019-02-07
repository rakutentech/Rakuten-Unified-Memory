/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   KVS.h
 * Author: paul.harvey
 *
 * Created on 2018/10/17, 14:00
 */

#ifndef __KVS_H__
#define __KVS_H__


#include "Value.h"

#include <stddef.h>
#include <memory>
#include <vector>

#include "hashring.h"

#include <Location.h>           // from RUM

typedef uint64_t Addr;

using namespace std;


class KVS {
public:

    KVS();
    KVS(std::string machines_csv_file_path);
    KVS(const KVS& orig);
    virtual ~KVS();
    
    /**
     * 
     */
    //void    add(KEY k, std::shared_ptr<Value> v);
    void    add(KEY k, Value* v);
    
    //std::shared_ptr<Value>   get(KEY k);
    Value*   get(KEY k);
    
    void    remove(KEY k);
    
    bool    contains(KEY k);
    
    size_t  size(void);

    void    end(void);
    
    void debug_dump_all_data();
private:
    hashring                 *ring;
    map<KEY, string>         *user_key_to_hash;
    map<KEY, Addr>           *user_key_to_ptr;
    size_t                   MD5_STR_LENGTH;
};

#endif /* __KVS_H__ */