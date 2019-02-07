/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   packet_type.h
 * Author: paul.harvey
 *
 * Created on 2018/11/15, 9:00
 */

#ifndef __PACKET_TYPE_H__
#define __PACKET_TYPE_H__

/**
 * This could totally be combined with packet.h, but it is separated to enable 
 * separate testing of the paxos
 */
enum pkt_type_e { 
    UNKNOWN = 0, 
    READ, 
    READ_HASH, 
    WRITE, 
    WRITE_HASH, 
    QUERY, 
    GET, GET_REPLY, 
    DATA, 
    ANSWER, 
    ALLOC, ALLOC_REPLY,
    DEALLOC,
    ALLOC_HASH, ALLOC_HASH_REPLY,
    DEALLOC_HASH, 
    END_PACKET };
typedef enum pkt_type_e pkt_type_t;

static const char *pkt_type_name[] = {
    "UNKNOWN", 
    "READ", 
    "READ_HASH", 
    "WRITE", 
    "WRITE_HASH", 
    "QUERY", 
    "GET", 
    "GET_REPLY", 
    "DATA", 
    "ANSWER", 
    "ALLOC",
    "ALLOC_REPLY",
    "DEALLOC",
    "ALLOC_HASH",
    "ALLOC_HASH_REPLY",
    "DEALLOC_HASH", 
    "END_PACKET"
};

#define PKT_STR(a) (pkt_type_name[(a)])
//------------------------------------------------------------------------------
#endif /* __PACKET_TYPE_H__ */

