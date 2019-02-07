/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   Node.cpp
 * Author: paul.harvey
 * 
 * Created on 2018/10/09, 11:32
 */


#include <cstring>

//#define NODE_DEBUG
#ifdef NODE_DEBUG
#define NODE_PRINTF(...) PRINTF(__VA_ARGS__)
#else
#define NODE_PRINTF(...)
#endif


#include "node.h"
//------------------------------------------------------------------------------
Node::Node(std::string ip, int port){
    _ip = ip;
    _port = port;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
Node::Node(const Node& orig) {
    _ip     = orig._ip;
    _port   = orig._port;
}
//------------------------------------------------------------------------------
Node::~Node() {
}
//------------------------------------------------------------------------------
size_t Node::marshal(std::shared_ptr<Node> me , char* buf, size_t len, bool encode){
    size_t pos = sizeof(size_t);
    
    if(me == NULL){
        if(encode){std::memcpy(buf, &pos, sizeof(size_t));}
        return pos;
    }
    
    size_t local_len = me->_ip.length();
    
    //encode the length of the IP
    if(encode){std::memcpy(buf + pos, &local_len, sizeof(size_t));}
    pos += sizeof(size_t);
    
    //encode the IP
    if(encode){std::memcpy(buf + pos, me->_ip.c_str(), local_len);}
    pos += local_len;
    
    //encode the port
    if(encode){std::memcpy(buf + pos, &me->_port, sizeof(int));}
    pos += sizeof(int);
    
    // finally, encode the value
    if(encode){std::memcpy(buf, &pos, sizeof(size_t));}
    
    NODE_PRINTF("node_marshal: node %s:%d (%ld)\n", me->_ip.c_str(), me->_port, pos);
    return pos;
}
//------------------------------------------------------------------------------
std::shared_ptr<Node> Node::demarshal(char* buf, size_t len, size_t* ret_pos){
    size_t pos = 0;
    size_t local_node_len, local_len;
    char*  local_ip;
    int     local_port;
    
    // get the length of the data
    std::memcpy(&local_len, buf, sizeof(size_t));
    pos += sizeof(size_t);
    
    if(local_len == sizeof(size_t)){
        // this value is null
        NODE_PRINTF("Node::demarshal :: NULL\n");
        *ret_pos = pos;
        return NULL;
    }
    
    std::memcpy(&local_node_len, buf + pos, sizeof(size_t));
    pos += sizeof(size_t);
    
    local_ip = (char*) malloc(local_node_len);
    std::memset(local_ip, '\0', local_node_len);
    
    std::memcpy(local_ip, buf + pos, local_node_len);
    pos += local_node_len;
    
    std::memcpy(&local_port, buf + pos, sizeof(int));
    pos += sizeof(int);
    
    //TODO("Node::demarshall : check that this is ok in c++ (copy)\n");
    std::shared_ptr<Node> ret_val = std::make_shared<Node>(std::string(local_ip, local_node_len), local_port);
    free(local_ip);
    
    *ret_pos = pos;
    //PRINTF("node_demarshal: NODE %s (%d)\n", ret_val->to_str().c_str(), ret_val->to_str().length());
    //PRINTF("node demarshal: bytes read %d\n", pos);
    return ret_val;
}
//------------------------------------------------------------------------------
//---------------------------------------------------
bool operator== (const Node a, const Node b){
    if(a._ip == b._ip){
        return a._port == b._port;
    }

    return a._ip == b._ip;
}
//---------------------------------------------------
 bool operator<  (const Node a, const Node b){
    if(a._ip == b._ip){
        return a._port < b._port;
    }

    return a._ip < b._ip;
 }
//---------------------------------------------------
 bool operator>  (const Node a, const Node b){
    if(a._ip == b._ip){
        return a._port > b._port;
    }

    return a._ip > b._ip;
 }
//---------------------------------------------------
 bool operator!= (const Node a, const Node b){
    if(a._ip == b._ip){
        return a._port != b._port;
    }

    return a._ip != b._ip;
 }
//---------------------------------------------------
 bool operator<= (const Node a, const Node b){
    if(a._ip == b._ip){
        return a._port <= b._port;
    }

    return a._ip <= b._ip;
 }
//---------------------------------------------------
 bool operator>= (const Node a, const Node b){
    if(a._ip == b._ip){
        return a._port >= b._port;
    }

    return a._ip >= b._ip;
 }
 //---------------------------------------------------
 std::string Node::to_str(void){
    if(this == NULL) return ""; 
    return _ip + ":" + std::to_string(_port);
 }
//---------------------------------------------------
bool Node::cmp_eq(std::shared_ptr<Node> a){
    if(_ip == a->_ip){
        return _port == a->_port;
    }

    return _ip == a->_ip;
}
//---------------------------------------------------
bool Node::cmp_lt(std::shared_ptr<Node> a){
    if(_ip == a->_ip){
        return _port < a->_port;
    }

    return _ip < a->_ip;
}
//---------------------------------------------------
bool Node::cmp_gt(std::shared_ptr<Node> a){
    if(_ip == a->_ip){
        return _port > a->_port;
    }

    return _ip > a->_ip;
}
//---------------------------------------------------
bool Node::cmp_neq(std::shared_ptr<Node> a){
    if(_ip == a->_ip){
        return _port != a->_port;
    }

    return _ip != a->_ip;
}
//---------------------------------------------------
bool Node::cmp_leq(std::shared_ptr<Node> a){
    if(_ip == a->_ip){
        return _port <= a->_port;
    }

    return _ip <= a->_ip;
}
//---------------------------------------------------
bool Node::cmp_geq(std::shared_ptr<Node> a){
    if(_ip == a->_ip){
        return _port >= a->_port;
    }

    return _ip >= a->_ip;
}
//---------------------------------------------------    