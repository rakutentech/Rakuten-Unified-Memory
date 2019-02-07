/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UDP_comms.h
 * Author: 
 * https://linux.m2osw.com/c-implementation-udp-clientserver
 *
 * Created on 2018/10/03, 13:29
 */

#ifndef UDP_COMMS_H
#define UDP_COMMS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <string>
//------------------------------------------------------------------------------
class udp_comms_runtime_error : public std::runtime_error
{
public:
    udp_comms_runtime_error(const char *w) : std::runtime_error(w) {}
};
//------------------------------------------------------------------------------
class UDP_comms {
public:
    
    UDP_comms(const std::string& addr, size_t port, bool listen);
    ~UDP_comms();
    
    int                 get_socket() const;
    int                 get_port() const;
    std::string         get_addr() const;

    int                 recv(char *msg, size_t max_size);
    int                 timed_recv(char *msg, size_t max_size, int max_wait_ms);

    int                 send(const char *msg, size_t size);
private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
    struct addrinfo *   f_addrinfo;
};

#endif /* UDP_COMMS_H */

