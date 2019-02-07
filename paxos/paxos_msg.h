#ifndef __PAXOS_MSG_H__
#define __PAXOS_MSG_H__

#include "util.h"
#include "paxos_id.h"
#include "node.h"
#include "Value.h"

enum MSG_ENUM_TYPE {prepare_enum, 
                    promise_enum, 
                    accept_req_enum, 
                    accepted_enum, 
                    resolution_enum, 
                    permission_nack_enum, 
                    suggestion_nack_enum, 
                    accepted_nack_enum, 
                    nack_enum, 
                    null_enum, 
                    propose_enum, 
                    sync_enum,
                    catchup_enum,
};
                   
static std::string enum_name[] = {
    "prepare_enum", 
    "promise_enum", 
    "accept_req_enum", 
    "accepted_enum", 
    "resolution_enum", 
    "permission_nack_enum", 
    "suggestion_nack_enum", 
    "accepted_nack_enum", 
    "nack_enum", 
    "null_enum", 
    "propose_enum",
    "sync_enum",
    "catchup_enum",
};
//---------------------------------------------------------
//---------------------------------------------------------
class PAXOS_MSG {
	MSG_ENUM_TYPE 	_type;
	std::shared_ptr<Node>		_from_uid;
        std::shared_ptr<Node>            _proposer_uid;

	std::shared_ptr<PAXOS_ID> 	_promised_proposal_id;
	std::shared_ptr<PAXOS_ID>	_proposal_id;
	std::shared_ptr<PAXOS_ID>	_last_accepted_id;

	std::shared_ptr<Value> 		_proposal_value;
	std::shared_ptr<Value>		_last_accepted_value;
	std::shared_ptr<Value> 		_v;
        
        size_t          _instance_number;

public:
	PAXOS_MSG();
        PAXOS_MSG(const PAXOS_MSG& orig);
        ~PAXOS_MSG();
        
	MSG_ENUM_TYPE get_type() 			{return _type;}

        std::shared_ptr<Node> get_from_uid()                              {return _from_uid;}
        void set_from_uid(std::shared_ptr<Node> n)                    {_from_uid = n;}
        
	std::shared_ptr<PAXOS_ID> get_proposal_id()                   {return _proposal_id;}
	std::shared_ptr<PAXOS_ID> get_last_accepted_id()             {return _last_accepted_id;}
	std::shared_ptr<PAXOS_ID> get_promised_proposal_id()         {return _promised_proposal_id;}
        
	std::shared_ptr<Value> get_last_accepted_value()		{return _last_accepted_value;}
        std::shared_ptr<Value> get_proposal_value()                      {return _proposal_value;}
        
        size_t get_instance_number()                    {return _instance_number;}
        
        // construct messages
        static std::shared_ptr<PAXOS_MSG> propose(std::shared_ptr<Value> v);
	static std::shared_ptr<PAXOS_MSG> prepare(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id);
	static std::shared_ptr<PAXOS_MSG> nack(std::shared_ptr<Node> from, std::shared_ptr<Node> proposer_uid, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> promised_proposal_id);
	static std::shared_ptr<PAXOS_MSG> promise(std::shared_ptr<Node> from, std::shared_ptr<Node> proposer_uid, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> last_accepted_id, std::shared_ptr<Value> last_accepted_val);
	static std::shared_ptr<PAXOS_MSG> accept(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> proposal_value);
	static std::shared_ptr<PAXOS_MSG> accepted(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> v);
	static std::shared_ptr<PAXOS_MSG> resolution(std::shared_ptr<Node> from, std::shared_ptr<Value> v);
        static std::shared_ptr<PAXOS_MSG> sync(std::shared_ptr<Node> from, size_t instance_number);
        static std::shared_ptr<PAXOS_MSG> catchup(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<Value> v);

        size_t marshal_len();
        size_t marshal(char* buf, size_t len);
        static std::shared_ptr<PAXOS_MSG> demarshal(char* buf, size_t len);
        
        void debug();

        
};

#endif /*__PAXOS_MSG_H__*/