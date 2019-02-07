/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TCP_comms.h
 * Author: paul.harvey
 *
 * Created on 2018/11/15, 11:38
 */

#ifndef TCP_COMMS_H
#define TCP_COMMS_H

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <string>

//------------------------------------------------------------------------------
class tcp_comms_runtime_error : public std::runtime_error
{
public:
    tcp_comms_runtime_error(const char *w) : std::runtime_error(w) {}
};
//------------------------------------------------------------------------------
class TCP_comms {
public:
    TCP_comms(const std::string& addr, size_t port, bool listen);
    TCP_comms(const TCP_comms& orig);
    virtual ~TCP_comms();
    
    int                 get_socket() const;
    int                 get_port() const;
    std::string         get_addr() const;

    int                 recv(char *msg, size_t max_size);
    int                 recv(int sock, char *msg, size_t max_size);
    int                 timed_recv(char *msg, size_t max_size, int max_wait_ms);

    int                 send(const char *msg, size_t size);
    int                 send(int sock, const char *msg, size_t size);
    
    int                 accept();
private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
    struct addrinfo *   f_addrinfo;
};

#endif /* TCP_COMMS_H */

