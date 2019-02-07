#ifndef __ACCEPTOR_H__
#define __ACCEPTOR_H__

#include <list>
#include "util.h"				// value and node
//---------------------------------------------------------
#include "paxos_id.h"
#include "paxos_msg.h"		// for the msg functions
#include "node.h"
//---------------------------------------------------------
class acceptor{
	Node*                   _network_uuid;
	PAXOS_ID*               _promised_id;
	PAXOS_ID*               _accepted_id;
	std::shared_ptr<Value>	_accepted_value;

public:
    acceptor(Node* me, PAXOS_ID* prom_id, PAXOS_ID* accpt_id, std::shared_ptr<Value> acpt_val);
    ~acceptor();
    
    PAXOS_MSG* receive_prepare(PAXOS_MSG msg, size_t instance_number);
    PAXOS_MSG* receive_accept(PAXOS_MSG msg);
};

#endif  /*__ACCEPTOR_H__*/