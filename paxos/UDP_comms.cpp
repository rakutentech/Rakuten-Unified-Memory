/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   UDP_comms.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/10/03, 13:29
 */

#include "UDP_comms.h"
#include <string.h>
#include <unistd.h>

#include "util.h"

//#define UDP_COMMS_DEBUG
#ifdef UDP_COMMS_DEBUG
#define UDP_COMMS_PRINTF(...) PRINTF(__VA_ARGS__)
#else
#define UDP_COMMS_PRINTF(...)
#endif

UDP_comms::UDP_comms(const std::string& addr, size_t port, bool listen) : f_port(port), f_addr(addr){
    
    UDP_COMMS_PRINTF("UDP_COMMS: new %s:%ld\n", addr.c_str(), port);
    
    char decimal_port[16];
    snprintf(decimal_port, sizeof(decimal_port), "%d", f_port);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    if(getaddrinfo(addr.c_str(), decimal_port, &hints, &f_addrinfo) != 0 || f_addrinfo == NULL)
    {
        ERROR("invalid address or port\n");
        throw udp_comms_runtime_error(("invalid address or port for UDP socket: \"" + addr + ":" + decimal_port + "\"").c_str());
    }
    f_socket = socket(f_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if(f_socket == -1)
    {
        freeaddrinfo(f_addrinfo);
        ERROR("could not create UDP socket\n");
        throw udp_comms_runtime_error(("could not create UDP socket for: \"" + addr + ":" + decimal_port + "\"").c_str());
    }
    
    int enable = 1;
    if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        ERROR("setsockopt(SO_REUSEADDR) failed");
    }

    if(listen){
        UDP_COMMS_PRINTF("UDP_COMMS: bind\n");
        if(bind(f_socket, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen) != 0)
        {
            freeaddrinfo(f_addrinfo);
            close(f_socket);
            ERROR("could not bind UDP socket with: %s:%ld\n", addr.c_str(), port);
            throw udp_comms_runtime_error(("could not bind UDP socket with: \"" + addr + ":" + decimal_port + "\"").c_str());
        }
    }
}
//------------------------------------------------------------------------------
UDP_comms::~UDP_comms(){
    UDP_COMMS_PRINTF("UDP_COMMS: kill %s:%ld\n", f_addr.c_str(), f_port);
    freeaddrinfo(f_addrinfo);
    close(f_socket);
}
//------------------------------------------------------------------------------
int UDP_comms::get_socket() const{
    return f_socket;
}
//------------------------------------------------------------------------------
int UDP_comms::get_port() const{
    return f_port;
}
//------------------------------------------------------------------------------
std::string UDP_comms::get_addr() const{
    return f_addr;
}
//------------------------------------------------------------------------------
int UDP_comms::recv(char *msg, size_t max_size){
    return ::recv(f_socket, msg, max_size, 0);
}
//------------------------------------------------------------------------------
int UDP_comms::timed_recv(char *msg, size_t max_size, int max_wait_ms)
{
    fd_set s;
    FD_ZERO(&s);
    FD_SET(f_socket, &s);
    struct timeval timeout;
    timeout.tv_sec = max_wait_ms / 1000;
    timeout.tv_usec = (max_wait_ms % 1000) * 1000;
    int retval = select(f_socket + 1, &s, NULL, NULL, &timeout);
    if(retval == -1)
    {
        // select() set errno accordingly
        return -1;
    }
    if(retval > 0)
    {
        // our socket has data
        return ::recv(f_socket, msg, max_size, 0);
    }

    // our socket has no data
    errno = EAGAIN;
    return -1;
}
//------------------------------------------------------------------------------
int UDP_comms::send(const char *msg, size_t size){
    size_t bytes;
    
    bytes = sendto(f_socket, msg, size, 0, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
    
    UDP_COMMS_PRINTF("UDP_COMMS: sent %ld/%ld bytes to %s:%d\n", bytes, size, f_addr.c_str(), f_port);
    return bytes;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------