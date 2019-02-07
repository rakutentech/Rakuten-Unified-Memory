#include "acceptor.h"
#include "UDP_comms.h"
//---------------------------------------------------------
acceptor::~acceptor(){
    
}
//---------------------------------------------------------
acceptor::acceptor(Node* me, PAXOS_ID* prom_id, PAXOS_ID* accpt_id, VALUE acpt_val){
        PRINTF("acceptor::acceptor\n");
	_network_uuid	= me;
	_promised_id	= prom_id;
	_accepted_id	= accpt_id;
	_accepted_value = acpt_val;
}
//---------------------------------------------------------
PAXOS_MSG* acceptor::receive_prepare(PAXOS_MSG msg, size_t instance_num){
    PRINTF("acceptor::receive_prepare\n");
    PAXOS_MSG* reply;
    
    if(_promised_id == NULL || msg.get_proposal_id()->cmp_geq(_promised_id)){
        PRINTF("acceptor:: newer msg\n");
        _promised_id = msg.get_proposal_id();
        reply = PAXOS_MSG::promise(_network_uuid, msg.get_from_uid(), instance_num, _promised_id, _accepted_id, _accepted_value);
    }
    else{
        PRINTF("acceptor:: old msg\n");
        reply =  PAXOS_MSG::nack(_network_uuid, msg.get_from_uid(), instance_num, msg.get_proposal_id(), _promised_id);
    }
    
    return reply;
    /*
    PRINTF("acceptor:: send\n");
    
    PRINTF("acceptor:: sendto : %s:%d\n", msg.get_from_uid()->get_ip().c_str(), msg.get_from_uid()->get_port());
    UDP_comms *c = new UDP_comms(msg.get_from_uid()->get_ip(), msg.get_from_uid()->get_port(), false);
    size_t buf_len  = reply->marshal_len();
    char*  buf      = (char*)malloc(buf_len);
    reply->marshal(buf, buf_len);
    
    c->send(buf, buf_len);
    
    free(buf);
    delete c;
    delete reply;
    */
}
//---------------------------------------------------------
PAXOS_MSG* acceptor::receive_accept(PAXOS_MSG msg){
    PAXOS_MSG* reply;
    if(msg.get_proposal_id()->cmp_geq(_promised_id)){
        _promised_id 	= msg.get_proposal_id();
        _accepted_id 	= msg.get_proposal_id();
        _accepted_value	= msg.get_proposal_value();


        PRINTF("acceptor::receive_accept: reply accepted\n");
        reply = PAXOS_MSG::accepted(_network_uuid, msg.get_proposal_id(), msg.get_proposal_value());
        /*
        size_t len     = reply->marshal_len();
        char*  tmp_buf = (char*) malloc(len);
        reply->marshal(tmp_buf, len);

        // send accept to all peers
        for(auto i : peers){
            PRINTF("acceptor::receive_accept: send accepted message to %s:%d\n", i->get_ip().c_str(), i->get_port());
            UDP_comms *c = new UDP_comms(i->get_ip(), i->get_port(), false);

            c->send(tmp_buf, len);

            delete c;
        }
        free(tmp_buf);
        delete reply;
        */ 
    }
    else{
        //return PAXOS_MSG::nack(_network_uuid, msg.get_from_uid(), msg.get_proposal_id(), _promised_id);
        TODO("I put a rubbish instance num....\n");
        reply = PAXOS_MSG::nack(_network_uuid, msg.get_from_uid(), -1, msg.get_proposal_id(), _promised_id);
    }
    return reply;
}
//---------------------------------------------------------