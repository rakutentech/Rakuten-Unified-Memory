/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Value.h
 * Author: paul.harvey
 *
 * Created on 2018/10/12, 9:41
 */

#ifndef __VALUE_H__
#define __VALUE_H__

#include "node.h"
//#include "user_value.h"

class Value {
public:
    Value(bool is_master_value, std::shared_ptr<Node> master_node, void* value, size_t len);
    Value(const Value& orig);
    virtual ~Value();
    
    bool                    is_master()     {return _is_new_master_value;}
    std::shared_ptr<Node>   get_master()    {return _master_node;}
    void*                   get_value()     {return _value;}
    size_t                  get_len()       {return _length;}
    
    static size_t marshal(std::shared_ptr<Value> me, char* buf, size_t len, bool encode);
    static std::shared_ptr<Value> demarshal(char* buf, size_t len, size_t* ret_pos);
    
    std::string to_str();
    
    bool cmp_eq(std::shared_ptr<Value> a);
    bool cmp_lt(std::shared_ptr<Value> a);
    bool cmp_gt(std::shared_ptr<Value> a);
    bool cmp_neq(std::shared_ptr<Value> a);
    bool cmp_leq(std::shared_ptr<Value> a);
    bool cmp_geq(std::shared_ptr<Value> a);
    
private:
    bool                    _is_new_master_value;
    std::shared_ptr<Node>   _master_node;
    void*                   _value;
    size_t                  _length;
};
#endif /* __VALUE_H__ */

