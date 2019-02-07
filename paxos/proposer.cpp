#include "proposer.h"
#include "UDP_comms.h"

//---------------------------------------------------------
proposer::~proposer(){
    delete _network_uid;
    delete _proposal_id;
    delete _highest_proposal_id;
    
    TODO("clear out _nacks_received\n");
    
}
//---------------------------------------------------------
proposer::proposer(Node* my_uid, size_t quorum_size){
    PRINTF("proposer::proposer\n");
    _network_uid = my_uid;
    _quorum_size = quorum_size;
    
    _proposal_id            = new PAXOS_ID(0, my_uid);
    _highest_proposal_id    = new PAXOS_ID(0, my_uid);
    
    _leader                 = false;
    _proposed_value         = VALUE_NULL;
    _highest_accepted_id    = NULL;
    _promises_received.clear();
    _nacks_received.clear();
    _current_accept_message = NULL;
    _current_prepare_message = NULL;
}
//---------------------------------------------------------
void proposer::propose_value(std::vector<Node*> peers, VALUE v){
    PRINTF("proposer::propose_value\n");
    PAXOS_MSG* m;
    size_t len;
    char* tmp_buf;
    UDP_comms *c;
    
    // master_strategy
    
    // resolution strategy
    
    if(_proposed_value == VALUE_NULL){
        _proposed_value = v;
        
        if(_leader){
            PRINTF("PROPOSER: PROPOSE_VALUE: we are leader, auto accept\n");
            
            delete _current_accept_message;
            _current_accept_message = PAXOS_MSG::accept(_network_uid, _proposal_id, v);
        }
        else{
            PRINTF("PROPOSER: PROPOSE_VALUE: not leader, propose\n");
            m       = PAXOS_MSG::prepare(_network_uid, _highest_proposal_id);
            len     = m->marshal_len();
            tmp_buf = (char*) malloc(len);
            m->marshal(tmp_buf, len);
            
            for(auto i : peers){
                PRINTF("PROPOSER: %s:%d\n", i->get_ip().c_str(), i->get_port());
                c = new UDP_comms(i->get_ip(), i->get_port(), false);
                c->send(tmp_buf, len);
            }
            
            free(tmp_buf);
            delete c;
            delete m;
        }
    }
}
//---------------------------------------------------------
PAXOS_MSG* proposer::prepare(){
	_leader         		= false;
        
        delete _highest_proposal_id;
	_highest_proposal_id 		= _proposal_id;
	_current_prepare_message 	= PAXOS_MSG::prepare(_network_uid, _proposal_id);
	_promises_received.clear();
	_nacks_received.clear();

	_proposal_id 			= new PAXOS_ID(_highest_proposal_id->get_count() + 1, _network_uid);
	
        PRINTF("proposer::prepare : new proposal_id = %s\n", _proposal_id->to_str().c_str());
        return _current_prepare_message;
}
//---------------------------------------------------------
void proposer::observe_proposal(PAXOS_ID* proposal_id){
    if(proposal_id > _highest_proposal_id){
        _highest_proposal_id = proposal_id;
    }
}
//---------------------------------------------------------
void proposer::receive_nack(PAXOS_MSG msg){
    observe_proposal(msg.get_promised_proposal_id());

    if(msg.get_proposal_id() == _proposal_id && !_nacks_received.empty()){

        TODO("proposer::receive_nack - Should make a copy of this \n");
        _nacks_received.insert(msg.get_from_uid());

        if(_nacks_received.size() == _quorum_size){
            // lost the leadership or failed to acquire it
            TODO("proposer::receive_nack - prepare??\n");
            PAXOS_MSG *tmp = prepare();
        }
    }
}
//---------------------------------------------------------
/**
 * Returns an Accept messages if a quorum of Promise messages is achieved
 * @param msg
 * @return 
 */
PAXOS_MSG* proposer::receive_promise(PAXOS_MSG msg){
    observe_proposal(msg.get_proposal_id());

    PRINTF("proposer::receive_promise\n");
    if(!_leader 
        && msg.get_proposal_id()->cmp_eq(_proposal_id)
        && _promises_received.count(msg.get_from_uid()) == 0){

        _promises_received.insert(msg.get_from_uid());

        if(msg.get_last_accepted_id() != NULL && msg.get_last_accepted_id()->cmp_gt(_highest_accepted_id)){
            _highest_accepted_id = msg.get_last_accepted_id();

            if(msg.get_last_accepted_value() != VALUE_NULL){
                _proposed_value = msg.get_last_accepted_value();
            }
        }

        if(_promises_received.size() == _quorum_size){
            _leader = true;

            // we now have consensus, as the group
            //if(_proposed_value != VALUE_NULL)
            TODO("BETTER FILTER FOR VALUE TYPES");
            {
                delete _current_accept_message;
                _current_accept_message = PAXOS_MSG::accept(_network_uid, _proposal_id, _proposed_value);
                return _current_accept_message;
                /*
                size_t len     = _current_accept_message->marshal_len();
                char*  tmp_buf = (char*) malloc(len);
                _current_accept_message->marshal(tmp_buf, len);
                
                // send accept to all peers
                for(auto i : peers){
                    PRINTF("PROPOSER: send accept message to %s:%d\n", msg.get_from_uid()->get_ip().c_str(), msg.get_from_uid()->get_port());
                    UDP_comms *c = new UDP_comms(i->get_ip(), i->get_port(), false);
                    
                    c->send(tmp_buf, len);
                    
                    delete c;
                }
                free(tmp_buf);
                */
            }
        }
    }
    return NULL;
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------