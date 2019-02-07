#ifndef __PROPOSER_H__
#define __PROPOSER_H__
#include "util.h"

#include "paxos_msg.h"
#include "paxos_role_base.h"
#include <set>
#include <vector>
//---------------------------------------------------------
class proposer {//: public paxos_role_base{
	bool 				_leader;
	std::shared_ptr<Value>		_proposed_value;
	PAXOS_ID* 			_proposal_id;
	PAXOS_ID*			_highest_proposal_id;
	PAXOS_ID*			_highest_accepted_id;
	std::set<Node*>                  _promises_received;
	std::set<Node*>                 _nacks_received;
	PAXOS_MSG*                      _current_prepare_message;
	PAXOS_MSG*                      _current_accept_message;
        
        size_t                  _quorum_size;
        Node*               _network_uid;

	public:
            ~proposer();
            proposer(Node* me, size_t quorum_size);
            
            PAXOS_ID*   get_proposal_id()     {return _proposal_id;}
            std::shared_ptr<Value>       get_proposed_value() {return _proposed_value;}
            
            void propose_value(std::vector<Node*> peers, std::shared_ptr<Value> v);

            PAXOS_MSG proposed_value(std::shared_ptr<Value> v);
            PAXOS_MSG* prepare();
            
            void receive_nack(PAXOS_MSG msg);
            PAXOS_MSG* receive_promise(PAXOS_MSG msg);
           
            void observe_proposal(PAXOS_ID* proposal_id);
};
#endif /*__PROPOSER_H__*/