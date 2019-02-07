/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   VALUE.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/10/17, 14:51
 */

#include "Value.h"

#include <stdlib.h>
#include <cstring>		//memcpy
#include <unistd.h>           // for the sleep

#define GET_LEN(a)		(*((size_t*)(a)))
#define GET_DATA(a)		((a) + sizeof(size_t))

#define DATA(a, offset)  	(*(((char*)(a)) + offset))

Value::Value(char* buf, size_t len) {
	//printf("VALUE:: len %ld\n", len);
    length  = len;
    buffer  = buf;
}

Value::Value(const Value& orig) {
}

Value::~Value() {
    free(buffer);
}

size_t Value::marshal_len(){
	return sizeof(size_t) + length;
}

void Value::marshal(char* buf){
	memcpy(buf, &length, sizeof(size_t));
	memcpy(buf + sizeof(size_t), buffer, length);
}
/*
std::shared_ptr<Value> Value::demarshal(char* buf){
	std::shared_ptr<Value> new_guy = 
						std::make_shared<Value>(GET_DATA(buf), GET_LEN(buf));
	return new_guy;
}
*/

Value* Value::demarshal(char* buf){
    /*
    printf("demarshal_len : %ld\n", GET_LEN(buf));
    for(size_t i = 0 ; i < GET_LEN(buf) + sizeof(size_t) ; i++){
        printf("[%ld] %c (%d)\n",i , DATA(buf, i), DATA(buf, i));
    }
    */
    Value* new_guy = new Value(GET_DATA(buf), GET_LEN(buf));    
    
	return new_guy;
}

void Value::compare_value(Value* a, Value* b){
	bool pass = true;

    printf("compare_value\n");
    size_t a_len = a->get_len();
    size_t b_len = b->get_len();

    if(a_len != b_len){
        printf("-----------------------------\n");
        printf("ERROR:: Lengths do not match!!\n");
        printf("A %ld || B %ld\n", a_len, b_len);
        printf("-----------------------------\n");
        usleep(1000000);
        pass = false;
    }

    auto a_ptr = a->get_buffer();
    auto b_ptr = b->get_buffer();

    for(size_t j = 0 ; j < a_len ; j++){
        if(*(a_ptr + j) != *(b_ptr + j)){
            printf("-----------------------------\n");
            printf("ERROR:: Values do not match!!\n");
            printf("     : LEN: A %ld || B %ld\n", a_len, b_len);
            printf("[%ld]: VAL: A %d || A %d\n", j, *(a_ptr + j), *(b_ptr + j));
            printf("-----------------------------\n");
            usleep(1000000);
            pass = false;
        }
    }
    printf("==> %s\n", pass?"PASS":"FAIL");
    printf("----------\n");
}