#ifndef __LEARNER_H__
#define __LEARNER_H__

#include "util.h"
#include "paxos_id.h"
#include "paxos_msg.h"
#include "paxos_role_base.h"
#include "node.h"

#include <map>
#include <set>
//------------------------------------------------------------------------------    
typedef struct proposal_status{
    size_t accept_count;
    size_t retain_count;
    std::map<std::string, std::shared_ptr<PAXOS_ID>> acceptors;
    std::shared_ptr<Value>  v;
}PROPOSAL_STATUS;
/*
struct proposal_status_comparator
{
    bool operator()(const User & left, const User & right) const
    {
        return (left.getName() > right.getName());
    }
};
 * */
//---------------------------------------------------------
class learner {//: public paxos_role_base{
    Node*                                   _network_uid;
    size_t                                  _quorum_size;
    std::map<std::string, PROPOSAL_STATUS*> _proposals;
    std::map<std::string, PAXOS_ID*>        _acceptors;
    std::shared_ptr<Value>                  _final_value;
    std::map<std::string, PAXOS_ID*>        _final_acceptors;
    PAXOS_ID*                               _final_proposal_id;
        
public:
    ~learner();
    learner(Node* my_uid, size_t quorum_size);
    
    PAXOS_MSG* receive_accepted(PAXOS_MSG msg);
};
//---------------------------------------------------------
#endif /*__LEARNER_H__*/