#include "learner.h"
#include "acceptor.h"
#include <cassert>

#include "UDP_comms.h"
//------------------------------------------------------------------------------
learner::~learner(){
    
}
//------------------------------------------------------------------------------
learner::learner(Node* my_uid, size_t quorum_size){
    PRINTF("learner::learner\n");
    _network_uid = my_uid;
    _quorum_size = quorum_size;
    
    _proposals.clear();
    _acceptors.clear();
    _final_value = VALUE_NULL;
    _final_acceptors.clear();
    _final_proposal_id = NULL;
}
//------------------------------------------------------------------------------    
#include <iostream>
PAXOS_MSG* learner::receive_accepted(PAXOS_MSG msg){
    PROPOSAL_STATUS* ps;
    PAXOS_ID* last_pn = NULL;

    if(_final_value != VALUE_NULL){
        if(msg.get_proposal_id()->cmp_geq(_final_proposal_id)
                && msg.get_proposal_value() == _final_value){
            TODO("learner::receive_accepted -  send EARLY resolution\n");
            
            _final_acceptors[msg.get_proposal_id()->to_str()] = msg.get_proposal_id();
            return PAXOS_MSG::resolution(_network_uid, _final_value);
        }
    }
    
    PRINTF("learner::receive_accepted : from %s\n", msg.get_from_uid()->to_str().c_str());
    if(_acceptors.count(msg.get_from_uid()->to_str()) > 0){
        last_pn  = _acceptors.at(msg.get_from_uid()->to_str());
        if(msg.get_proposal_id()->cmp_leq(last_pn)){
            // old message
            PRINTF("learner::receive_accepted:  OLD MESSAGE - ignore\n");
            return NULL;
        }
    }

    // record the message
    _acceptors[msg.get_from_uid()->to_str()] = msg.get_proposal_id();

    if(last_pn != NULL){
       ps = _proposals.at(last_pn->to_str());
       ps->retain_count--;
       ps->acceptors.erase(msg.get_from_uid()->to_str());
       if(ps->retain_count == 0){
           _proposals.erase(last_pn->to_str());
       }
    }

    if(_proposals.count(msg.get_proposal_id()->to_str()) == 0){
        PROPOSAL_STATUS* tmp_ps = new PROPOSAL_STATUS;
        tmp_ps->accept_count = 0;
        tmp_ps->retain_count = 0;
        tmp_ps->v            = msg.get_proposal_value();
        tmp_ps->acceptors.clear();

        TODO("learner::receive_accepted : heck how memory is allocated here\n");
        _proposals[msg.get_proposal_id()->to_str()] = tmp_ps;
    }

    ps = _proposals[msg.get_proposal_id()->to_str()];
    PRINTF("learner::receive_accepted : proposed %p || current %p\n", msg.get_proposal_value(), ps->v);
    PRINTF("learner::receive_accepted : proposed %s || current %s\n", msg.get_proposal_value().c_str(), ps->v.c_str());
    TODO("learner::receive_accepted : make the value assertion generic || %d\n", msg.get_proposal_value().compare( ps->v));
    //assert(!msg.get_proposal_value().compare( ps->v));

    ps->accept_count++;
    ps->retain_count++;
    ps->acceptors.insert(std::pair<std::string, PAXOS_ID*>(msg.get_from_uid()->to_str(),  NULL));

    if(ps->accept_count == _quorum_size){
        _final_proposal_id  = msg.get_proposal_id();
        _final_value        = msg.get_proposal_value();
        _final_acceptors    = ps->acceptors;
        TODO("address the memory here\n");
        _proposals.clear();
        _acceptors.clear();

        TODO("learner::receive_accepted -  send resolution\n");
        // send accept to all peers
        for(auto i : _final_acceptors){

            std::cout << "final acceptor "  << i.first << " "  << "\n";
        }
        
        return PAXOS_MSG::resolution(_network_uid, _final_value);
    }
    return NULL;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------