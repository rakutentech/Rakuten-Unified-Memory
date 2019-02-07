#include <cstring>

#include "paxos_id.h"

//#define PAXID_DEBUG
#ifdef PAXID_DEBUG
#define PAXID_PRINTF(...) PRINTF(__VA_ARGS__)
#else
#define PAXID_PRINTF(...)
#endif
//---------------------------------------------------
//PAXOS_ID::PAXOS_ID(){
//	_id 	= 0;
//	_node	= Node("", 0);
//}
//---------------------------------------------------
PAXOS_ID::PAXOS_ID(size_t a, std::shared_ptr<Node> b){
    _id 	= a;
    _node	= b;
}
//---------------------------------------------------
PAXOS_ID::PAXOS_ID(const PAXOS_ID& orig){
    _id     = orig._id;
    _node   = orig._node;
}
//---------------------------------------------------
PAXOS_ID::~PAXOS_ID(){
    //delete _node;
}
/*
//---------------------------------------------------
bool PAXOS_ID::operator== (const PAXOS_ID a){
	if(a._id == _id){
		return a._node == _node;
	}

	return a._id == _id;
}
//---------------------------------------------------
bool PAXOS_ID::operator< (const PAXOS_ID a){
	if(a._id == _id){
		return a._node < _node;
	}

	return a._id < _id;
}
//---------------------------------------------------
bool PAXOS_ID::operator> (const PAXOS_ID a){
	if(a._id == _id){
		return a._node > _node;
	}

	return a._id > _id;
}
//---------------------------------------------------
bool PAXOS_ID::operator!= (const PAXOS_ID a){
	if(a._id == _id){
		return a._node != _node;
	}

	return a._id != _id;
}
//---------------------------------------------------
bool PAXOS_ID::operator>= (const PAXOS_ID a){
	if(a._id == _id){
		return a._node >= _node;
	}

	return a._id >= _id;
}
//---------------------------------------------------
bool PAXOS_ID::operator<= (const PAXOS_ID a){
	if(a._id == _id){
		return a._node <= _node;
	}

	return a._id <= _id;
}
*/
//---------------------------------------------------
std::string PAXOS_ID::to_str(void){
    if(this == NULL){
        return "";
    }
    else{
        return (std::to_string(_id) + " . " + _node->to_str());
    }
}
//---------------------------------------------------
bool operator== (const PAXOS_ID a, const PAXOS_ID b){
    if(a._id == b._id){
        return a._node == b._node;
    }

    return a._id == b._id;
}
//---------------------------------------------------
 bool operator<  (const PAXOS_ID a, const PAXOS_ID b){
    if(a._id == b._id){
        return a._node < b._node;
    }

    return a._id < b._id;
 }
//---------------------------------------------------
 bool operator>  (const PAXOS_ID a, const PAXOS_ID b){
    if(a._id == b._id){
        return a._node > b._node;
    }

    return a._id > b._id;
 }
//---------------------------------------------------
 bool operator!= (const PAXOS_ID a, const PAXOS_ID b){
    if(a._id == b._id){
        return a._node != b._node;
    }

    return a._id != b._id;
 }
//---------------------------------------------------
 bool operator<= (const PAXOS_ID a, const PAXOS_ID b){
    if(a._id == b._id){
        return a._node <= b._node;
    }

    return a._id <= b._id;
 }
//---------------------------------------------------
 bool operator>= (const PAXOS_ID a, const PAXOS_ID b){
    if(a._id == b._id){
        return a._node >= b._node;
    }

    return a._id >= b._id;
 }
//---------------------------------------------------
bool PAXOS_ID::cmp_eq(std::shared_ptr<PAXOS_ID> a){
    if(_id == a->_id){
        return _node->cmp_eq(a->_node);
    }

    return _id == a->_id;
}
//---------------------------------------------------
bool PAXOS_ID::cmp_lt(std::shared_ptr<PAXOS_ID> a){
    if(_id == a->_id){
        return _node->cmp_lt(a->_node);
    }

    return _id < a->_id;
}
//---------------------------------------------------
bool PAXOS_ID::cmp_gt(std::shared_ptr<PAXOS_ID> a){
    if(a == NULL) return true;
    if(_id == a->_id){
        return _node->cmp_gt(a->_node);
    }

    return _id > a->_id;
}
//---------------------------------------------------
bool PAXOS_ID::cmp_neq(std::shared_ptr<PAXOS_ID> a){
    if(_id == a->_id){
        return _node->cmp_neq(a->_node);
    }

    return _id != a->_id;
}
//---------------------------------------------------
bool PAXOS_ID::cmp_leq(std::shared_ptr<PAXOS_ID> a){
    if(_id == a->_id){
        return _node->cmp_leq(a->_node);
    }

    return _id <= a->_id;
}
//---------------------------------------------------
bool PAXOS_ID::cmp_geq(std::shared_ptr<PAXOS_ID> a){
    if(a == NULL) return true;
    if(_id == a->_id){
        return _node->cmp_geq(a->_node);
    }

    return _id >= a->_id;
}
//---------------------------------------------------
size_t PAXOS_ID::marshal(std::shared_ptr<PAXOS_ID> me, char* buf, size_t len, bool encode){
    size_t pos = 0;
            
    // the length value is copied at the end
    pos += sizeof(size_t);
    
    if(me == NULL){
        if(encode){ std::memcpy(buf, &pos, sizeof(size_t));}
        return pos;
    }
    
    PAXID_PRINTF("PAXOS_ID::marshall: %ld.'%s'\n", me->_id, me->_node->to_str().c_str());
    
    // the id
    if(encode){std::memcpy(buf + pos, &me->_id, sizeof(size_t));}
    pos += sizeof(size_t);
    
    // the NODE
    pos += Node::marshal(me->_node, buf + pos, len, encode);
    
    // copy the length
    if(encode){ std::memcpy(buf, &pos, sizeof(size_t));}
    
    PAXID_PRINTF("PAXOS_ID::marshall: total %ld\n", pos);
#if 0
    for(auto i = 0 ; encode && i < pos ; i++){
        printf("[%d] %c\n", i, *(buf + i));
    }
#endif
    return pos;
}
 //---------------------------------------------------
std::shared_ptr<PAXOS_ID> PAXOS_ID::demarshal(char* buf, size_t* len){
    size_t pos;
    size_t tmp_pos;
    size_t local_len;
    size_t local_id;
    std::shared_ptr<Node>   tmp_node;
    
    // the length
    std::memcpy(&local_len, buf, sizeof(size_t));
    pos = sizeof(size_t);
    
    if(local_len == sizeof(size_t)){
        // this value is null
        *len = pos;
        return NULL;
    }
    
    // the id
    std::memcpy(&local_id, buf + pos, sizeof(size_t));
    pos += sizeof(size_t);
    PAXID_PRINTF("PAXOS_ID::demarshal: id %ld\n", local_id);
    
    // the NODE
    tmp_pos = 0;
    tmp_node = Node::demarshal(buf + pos, *len, &tmp_pos);
    pos += tmp_pos;
    
    PAXID_PRINTF("PAXOS_ID::demarshal: node %s (%ld)\n", tmp_node->to_str().c_str(), tmp_node->to_str().length());
    
    PAXID_PRINTF("PAXOS_ID::demarshal: marshaled len %ld\n", local_len);
    *len += pos;
    return std::make_shared<PAXOS_ID>(local_id, tmp_node);
}
//---------------------------------------------------