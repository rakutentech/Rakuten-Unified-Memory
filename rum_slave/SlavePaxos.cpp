/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   SlavePaxos.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/11/13, 15:34
 */

#include "SlavePaxos.h"

#include "../paxos/paxos_msg.h"
#include "../paxos/TCP_comms.h"
#include "../paxos/Value.h"
//#include "../rum_master/packet.h"

#define REPLY_BUF_SIZE  1024
#define RETRY_LIMIT     20
//------------------------------------------------------------------------------
SlavePaxos::SlavePaxos(std::vector<std::shared_ptr<Node>> slave_s) {
    slaves = slave_s;
}
//------------------------------------------------------------------------------
SlavePaxos::SlavePaxos(const SlavePaxos& orig) {
}
//------------------------------------------------------------------------------
SlavePaxos::~SlavePaxos() {
}
//------------------------------------------------------------------------------
void SlavePaxos::ship_and_send(slave_paxos_msg *real_msg){
     char reply[REPLY_BUF_SIZE];
    TCP_comms *comms;
    bool done = false;
    size_t pos;
    size_t received_bytes;
    size_t index = 0;
    std::shared_ptr<Node> n;
    size_t retry_count = 0;
    
    // our app specific value
    size_t msg_len = real_msg->marshal(NULL, 0, false);
    char* msg_buf = (char*)malloc(msg_len);
    real_msg->marshal(msg_buf, msg_len, true);
    real_msg->dump();
    
#if 0
    PRINTF("SlavePaxos::alloc:  %lx, %ld bytes\n", (Addr) msg_buf, msg_len);
    for(size_t i = 0 ; i < msg_len ; i++){
        printf("[%ld] = %d\n", i, msg_buf[i]);
    }
    
    // test the debug
    size_t demarshaled_bytes = 0;
    slave_paxos_msg *msg = slave_paxos_msg::demarshal((char*)msg_buf, msg_len, &demarshaled_bytes);
    msg->dump();
#endif    
    
    // make the paxos message
    std::shared_ptr<Value> prep_val = std::make_shared<Value>(false, nullptr, msg_buf, msg_len);
    std::shared_ptr<Node> me = std::make_shared<Node>("127.0.0.1", 45670);
    std::shared_ptr<PAXOS_MSG> start = PAXOS_MSG::propose(prep_val);
    start->set_from_uid(me);
    
    //package it up
    size_t len = start->marshal_len();   
    char* buf  = (char*)malloc(len);
    start->marshal(buf, len);
    
    // choose someone to ask
    n = slaves.at(index);
    index = (index + 1 ) % slaves.size();
    
    // ship it
    comms = new TCP_comms(n->get_ip(), n->get_port(), false);
    comms->send(buf, len);
    while(!done){
        if((received_bytes = comms->recv(reply, REPLY_BUF_SIZE)) > 1){
            printf("nack : %ld\n", received_bytes);
            
            if(retry_count++ > RETRY_LIMIT){
                ERROR("Could not contact the PAXOS network\n");
                return;
            }
            
            // need to send elsewhere
            n = Node::demarshal(reply, REPLY_BUF_SIZE, &pos);
            delete comms;
            
            if(n == NULL){
                n = slaves.at(index);
                index = (index + 1 ) % slaves.size();
                comms = new TCP_comms(n->get_ip(), n->get_port(), false);
            }
            else{
                comms = new TCP_comms(n->get_ip(), n->get_port() + 1, false);
            }
            // ship it
            
            comms->send(buf, len);
            usleep(100000);
        }
        else{
            printf("ack\n");
            done = true;
        }
    }
    
    // tidy the data
    delete comms;
    free(buf);
    free(msg_buf);
}
//------------------------------------------------------------------------------
void SlavePaxos::alloc(size_t size, Addr slave_ptr){
    slave_paxos_msg *real_msg = new slave_paxos_msg(size, slave_ptr);
    ship_and_send(real_msg);
    delete real_msg;
  }
//------------------------------------------------------------------------------
void SlavePaxos::alloc_hash(size_t size, HASH_TYPE hash, Addr slave_ptr){
    slave_paxos_msg *real_msg = new slave_paxos_msg(size, hash, slave_ptr);
    ship_and_send(real_msg);
    delete real_msg;
}
//------------------------------------------------------------------------------
void SlavePaxos::dealloc(Addr slave_ptr){
    slave_paxos_msg *real_msg = new slave_paxos_msg(slave_ptr);
    ship_and_send(real_msg);
    delete real_msg;
}
//------------------------------------------------------------------------------
void SlavePaxos::dealloc_hash(HASH_TYPE hash){
    slave_paxos_msg *real_msg = new slave_paxos_msg(hash);
    ship_and_send(real_msg);
    delete real_msg;
}
//------------------------------------------------------------------------------
void SlavePaxos::write(Addr ptr, size_t offset, size_t length, char* data){
    slave_paxos_msg *real_msg = new slave_paxos_msg(ptr, offset, length, data);
    ship_and_send(real_msg);
    delete real_msg;
}
//------------------------------------------------------------------------------

void SlavePaxos::write_hash(HASH_TYPE hash, size_t offset, size_t length, char* data){
    slave_paxos_msg *real_msg = new slave_paxos_msg(hash, offset, length, data);
    ship_and_send(real_msg);
    delete real_msg;
}
//------------------------------------------------------------------------------
