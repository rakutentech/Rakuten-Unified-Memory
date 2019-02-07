#ifndef __PAXOS_ID_H__
#define __PAXOS_ID_H__

#include <string>
#include "util.h"
#include "node.h"

class PAXOS_ID{
size_t 	_id;
std::shared_ptr<Node>	_node;

friend bool operator== (const PAXOS_ID a, const PAXOS_ID b);
friend bool operator<  (const PAXOS_ID a, const PAXOS_ID b);
friend bool operator>  (const PAXOS_ID a, const PAXOS_ID b);
friend bool operator!= (const PAXOS_ID a, const PAXOS_ID b);
friend bool operator<= (const PAXOS_ID a, const PAXOS_ID b);
friend bool operator>= (const PAXOS_ID a, const PAXOS_ID b);	

public:
        PAXOS_ID(size_t a, std::shared_ptr<Node> b);
        PAXOS_ID(const PAXOS_ID& orig);
        ~PAXOS_ID();
        
        size_t get_count() {return _id;}
        std::shared_ptr<Node> get_node()     {return _node;}
        
        bool cmp_eq(std::shared_ptr<PAXOS_ID> a);
        bool cmp_lt(std::shared_ptr<PAXOS_ID> a);
        bool cmp_gt(std::shared_ptr<PAXOS_ID> a);
        bool cmp_neq(std::shared_ptr<PAXOS_ID> a);
        bool cmp_leq(std::shared_ptr<PAXOS_ID> a);
        bool cmp_geq(std::shared_ptr<PAXOS_ID> a);
        
        std::string to_str(void);

        static size_t marshal(std::shared_ptr<PAXOS_ID> me, char* buf, size_t len, bool encode);
        static std::shared_ptr<PAXOS_ID> demarshal(char* buf, size_t* len);
        
};

#endif /*__PAXOS_ID_H__*/