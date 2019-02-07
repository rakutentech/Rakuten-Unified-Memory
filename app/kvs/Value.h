/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Value.h
 * Author: paul.harvey
 *
 * Created on 2018/10/17, 14:51
 */

#ifndef VALUE_H
#define VALUE_H

#include <stddef.h>
#include <memory>

class Value {
public:
    Value(char* buf, size_t len);
    Value(const Value& orig);
    virtual ~Value();
    
    size_t      get_len()       {return length;}
    const char* get_buffer()    {return buffer;}       

    size_t							marshal_len();
    void							marshal(char* buf);
    //static std::shared_ptr<Value> 	demarshal(char* buf);
    static Value*   demarshal(char* buf);
    static void     compare_value(Value* a, Value* b);
private:
    size_t  length;
    char*   buffer;
};

#endif /* VALUE_H */


