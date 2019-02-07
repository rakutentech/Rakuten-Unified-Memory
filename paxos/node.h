/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Node.h
 * Author: paul.harvey
 *
 * Created on 2018/10/09, 11:32
 */

#ifndef __NODE_H__
#define __NODE_H__
#include "util.h"

#define NODE_NULL (Node("", -1))

class Node {
    std::string _ip;
    int _port;
    
    friend bool operator== (const Node a, const Node b);
    friend bool operator<  (const Node a, const Node b);
    friend bool operator>  (const Node a, const Node b);
    friend bool operator!= (const Node a, const Node b);
    friend bool operator<= (const Node a, const Node b);
    friend bool operator>= (const Node a, const Node b);	

    
public:
    Node(std::string ip, int port);
    Node(const Node& orig);
    
    virtual ~Node();
    std::string to_str(void);
    
    std::string get_ip()    {return _ip;}
    int         get_port()   {return _port;}
    
    bool cmp_eq(std::shared_ptr<Node> a);
    bool cmp_lt(std::shared_ptr<Node> a);
    bool cmp_gt(std::shared_ptr<Node> a);
    bool cmp_neq(std::shared_ptr<Node> a);
    bool cmp_leq(std::shared_ptr<Node> a);
    bool cmp_geq(std::shared_ptr<Node> a);
    
    
    static size_t marshal(std::shared_ptr<Node>, char* buf, size_t len, bool encode);
    static std::shared_ptr<Node> demarshal(char* buf, size_t len, size_t* ret_pos);
private:

};

#endif /* __NODE_H__ */

