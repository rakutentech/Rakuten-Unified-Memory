/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Value.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/10/12, 9:41
 */

#include "Value.h"
#include "util.h"
#include <string.h>
#include <cstring>
//------------------------------------------------------------------------------
//#define VALUE_DEBUG
#ifdef VALUE_DEBUG
#define VALUE_PRINTF(...) PRINTF(__VA_ARGS__)
#else
#define VALUE_PRINTF(...)
#endif
//------------------------------------------------------------------------------
Value::Value(bool contains_master, std::shared_ptr<Node> master_node, void* value, size_t len) {
    _is_new_master_value    = contains_master;
    _value                  = value;
    _length                 = len;
    _master_node            = master_node;
}
//------------------------------------------------------------------------------
Value::Value(const Value& orig) {
    _is_new_master_value    = orig._is_new_master_value;
    _master_node            = orig._master_node;
    _value                  = orig._value;
}
//------------------------------------------------------------------------------
Value::~Value() {
    //delete _master_node;
}
//------------------------------------------------------------------------------
size_t Value::marshal(std::shared_ptr<Value> me, char* buf, size_t len, bool encode){
    
    size_t pos = sizeof(size_t);
 
    
    // if null, just write the length
    if(me == NULL){
        if(encode){ std::memcpy(buf, &pos, sizeof(size_t));}
        VALUE_PRINTF("Value::marshall :: NULL\n");
        return pos;
    }
    
    //PRINTF("Value::marshal :: [ %s | %s] \n", _is_new_master_value?"MASTER":"VALUE",_value.c_str());
    // encode the master
    if(encode) {std::memcpy(buf + pos, &me->_is_new_master_value, sizeof(bool));}
    pos += sizeof(bool);
    //PRINTF("Value::marshal :: is_master %d\n", _is_new_master_value);

    // encode the value
    if(encode){std::memcpy(buf + pos, &(me->_length), sizeof(size_t));}
    pos += sizeof(size_t);
    if(encode){std::memcpy(buf + pos, me->_value, me->_length);}
    pos += me->_length;
#if 0    
    PRINTF("Value::marshal:: %lx, %ld bytes\n", me->_value, me->_length);
    for(size_t i = 0 ; i <  me->_length ; i++){
        printf("[%ld] = %d\n", i,  ((char*)me->_value)[i]);
    }
#endif    
    //master_node
    pos += Node::marshal(me->_master_node, buf + pos, len, encode);
    

    // finally we encode the length
    if(encode){std::memcpy(buf, &pos, sizeof(size_t));}

    VALUE_PRINTF("Value::marshal :: [ %s | <***> ] \n", me->_is_new_master_value?"MASTER":"VALUE");
    return pos;
}
//------------------------------------------------------------------------------
std::shared_ptr<Value> Value::demarshal(char* buf, size_t len, size_t* ret_pos){
    size_t      pos = 0;
    size_t      local_value_len;
    size_t      tmp_val = 0;
    char*       local_value;
    bool        is_master;
    std::shared_ptr<Node>   new_node;
    std::shared_ptr<Value>  new_guy;
    size_t local_len;
    
    // get the length of the data
    std::memcpy(&local_len, buf, sizeof(size_t));
    pos += sizeof(size_t);
    
    if(local_len == sizeof(size_t)){
        // this value is null
        VALUE_PRINTF("Value::demarshal :: NULL\n");
        *ret_pos = pos;
        return NULL;
    }
    
    // decode the master
    std::memcpy(&is_master, buf + pos, sizeof(bool));
    pos += sizeof(bool);
    //PRINTF("Value::demarshal :: is_master %d\n", is_master);

    // decode the value
    std::memcpy(&local_value_len, buf + pos, sizeof(size_t));
    pos += sizeof(size_t);
    if((local_value = (char*)malloc(local_value_len)) == NULL){
        ERROR("VALUE::demarshal: can not allocate memory\n");
    }
    memset(local_value, 0, local_value_len);
    std::memcpy(local_value, buf + pos, local_value_len);
    pos += local_value_len;
    
#if 0
    PRINTF("Value::marshal:: %lx, %ld bytes\n", local_value, local_value_len);
    for(size_t i = 0 ; i < local_value_len ; i++){
        printf("[%ld] = %d\n", i,  ((char*)local_value)[i]);
    }
#endif    
    //master node
    //TODO("Value::demarshal : check that this is ok in c++ (copy)\n");
    new_node = Node::demarshal(buf + pos, len, &tmp_val);
    //new_guy = std::make_shared<Value>(is_master, new_node, std::string(local_value, local_value_len));
    new_guy = std::make_shared<Value>(is_master, new_node, local_value, local_value_len);
    pos += tmp_val;
    if(local_value_len > 0){
        TODO("MEMLEAK!!!! Value::marshal: free local len\n");
    }
    //free(local_value);
    
    *ret_pos = pos;
    VALUE_PRINTF("Value::demarshal :: [ %s  | %s] \n", new_guy->_is_new_master_value?"MASTER":"VALUE", "new_guy->_value.c_str()");
    return new_guy;
}
//------------------------------------------------------------------------------
std::string Value::to_str(){
    if(this == NULL){
        return "";
    }
    
    if(_is_new_master_value){
        return "[" + _master_node->to_str() + " | ]";
    }
    else{
        //return "[ | " + "_value" + "]";
        return "[ | _value ]";
    }
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool Value::cmp_eq(std::shared_ptr<Value> a){
    return _value == a->_value;
}
//---------------------------------------------------
bool Value::cmp_lt(std::shared_ptr<Value> a){
    return _value < a->_value;
}
//---------------------------------------------------
bool Value::cmp_gt(std::shared_ptr<Value> a){
    return _value > a->_value;
}
//---------------------------------------------------
bool Value::cmp_neq(std::shared_ptr<Value> a){
    return _value != a->_value;
}
//---------------------------------------------------
bool Value::cmp_leq(std::shared_ptr<Value> a){
    return _value <= a->_value;
}
//---------------------------------------------------
bool Value::cmp_geq(std::shared_ptr<Value> a){
    return _value >= a->_value;
}
//---------------------------------------------------    