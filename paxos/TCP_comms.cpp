/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   TCP_comms.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/11/15, 11:38
 */

#include "TCP_comms.h"
#include <string.h>
#include <unistd.h>
#include <cstring>

#include "util.h"
#define MAX_PENDING  10

//------------------------------------------------------------------------------
TCP_comms::TCP_comms(const std::string& addr, size_t port, bool listen) : f_port(port), f_addr(addr) {
    PRINTF("TCP_comms: new %s:%ld\n", addr.c_str(), port);
    
    char decimal_port[16];
    snprintf(decimal_port, sizeof(decimal_port), "%d", f_port);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if(getaddrinfo(addr.c_str(), decimal_port, &hints, &f_addrinfo) != 0 || f_addrinfo == NULL)
    {
        ERROR("invalid address or port\n");
        throw tcp_comms_runtime_error(("invalid address or port for TCP socket: \"" + addr + ":" + decimal_port + "\"").c_str());
    }
    f_socket = socket(f_addrinfo->ai_family, SOCK_STREAM, IPPROTO_TCP);
    if(f_socket == -1)
    {
        freeaddrinfo(f_addrinfo);
        ERROR("could not create TCP socket\n");
        throw tcp_comms_runtime_error(("could not create TCP socket for: \"" + addr + ":" + decimal_port + "\"").c_str());
    }
    
    int enable = 1;
    if (setsockopt(f_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0){
        ERROR("setsockopt(SO_REUSEADDR) failed");
    }

    if(listen){
        PRINTF("TCP_COMMS: bind\n");
        if(::bind(f_socket, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen) != 0)
        {
            freeaddrinfo(f_addrinfo);
            close(f_socket);
            ERROR("could not bind TCP socket with: %s:%ld\n", addr.c_str(), port);
            throw tcp_comms_runtime_error(("could not bind TCP socket with: \"" + addr + ":" + decimal_port + "\"").c_str());
        }
        
        if(::listen(f_socket, MAX_PENDING) < 0)
        {
            freeaddrinfo(f_addrinfo);
            close(f_socket);
            ERROR("could not listen TCP socket with: %s:%ld\n", addr.c_str(), port);
            throw tcp_comms_runtime_error(("could not listen TCP socket with: \"" + addr + ":" + decimal_port + "\"").c_str());
        }
    }
}
//------------------------------------------------------------------------------
TCP_comms::TCP_comms(const TCP_comms& orig) {
}
//------------------------------------------------------------------------------
TCP_comms::~TCP_comms() {
    PRINTF("TCP_comms: kill %s:%d\n", f_addr.c_str(), f_port);
    freeaddrinfo(f_addrinfo);
    close(f_socket);
}
//------------------------------------------------------------------------------
int TCP_comms::get_socket() const{
    return f_socket;
}
//------------------------------------------------------------------------------
int TCP_comms::get_port() const{
    return f_port;
}
//------------------------------------------------------------------------------
std::string TCP_comms::get_addr() const{
    return f_addr;
}
//------------------------------------------------------------------------------
int TCP_comms::accept(){
    int sock;
    if((sock = ::accept(f_socket, f_addrinfo->ai_addr, &(f_addrinfo->ai_addrlen))) < 0){
        freeaddrinfo(f_addrinfo);
        close(f_socket);
        ERROR("could not accept TCP socket with: %s:%d\n", f_addr.c_str(), f_port);
        throw tcp_comms_runtime_error(("could not accept TCP socket with: \"" + f_addr + ":" + std::to_string(f_port) + "\"").c_str());
    }
    return sock;
}
//------------------------------------------------------------------------------
int TCP_comms::recv(int sock, char *msg, size_t max_size){
    return ::recv(sock, msg, max_size, 0);
}
//------------------------------------------------------------------------------
int TCP_comms::recv(char *msg, size_t max_size){
    return ::recv(f_socket, msg, max_size, 0);
}
//------------------------------------------------------------------------------
int TCP_comms::send(const char *msg, size_t size){
    size_t bytes;
    
    if(::connect(f_socket, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen) < 0)
    {
        freeaddrinfo(f_addrinfo);
        close(f_socket);
        ERROR("could not connect TCP socket with: %s:%d\n", f_addr.c_str(), f_port);
        throw tcp_comms_runtime_error(("could not connect TCP socket with: \"" + f_addr + ":" + std::to_string(f_port) + "\"").c_str());
    }
    
    //bytes = ::sendto(f_socket, msg, size, 0, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
    bytes = ::send(f_socket, msg, size, 0);
    
    PRINTF("TCP_COMMS: sent %ld/%ld bytes to %s:%d\n", bytes, size, f_addr.c_str(), f_port);
    return bytes;
}
//------------------------------------------------------------------------------
int TCP_comms::send(int sock, const char *msg, size_t size){
    size_t bytes;
     //bytes = ::sendto(f_socket, msg, size, 0, f_addrinfo->ai_addr, f_addrinfo->ai_addrlen);
    bytes = ::send(sock, msg, size, 0);
    
    PRINTF("TCP_COMMS: sent %ld/%ld bytes to %s:%d\n", bytes, size, f_addr.c_str(), f_port);
    return bytes;
}
//------------------------------------------------------------------------------