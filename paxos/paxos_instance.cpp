#include "paxos_instance.h"
#include "acceptor.h"
#include "proposer.h"
#include "UDP_comms.h"
#include "TCP_comms.h"
#include "paxos_timer.h"

#include <stdlib.h>         // for rand
#include <random>           // for the better random
#include <fstream>          // for files
#include <iostream>
#include <string.h>         // for memset

#define VALUE_STR(v)        ((v)?(v)->to_str().c_str():"NULL")    
//---------------------------------------------------------
/*
paxos_instance::paxos_instance(){
    TODO("Why is this being called\n");
    
    _proposer = proposer(NODE_NULL, 0);
    _acceptor = acceptor(NODE_NULL, PAXOS_ID(0, NODE_NULL), PAXOS_ID(0, NODE_NULL), NULL);
    _learner  = learner(NODE_NULL, 0);
}
 */ 
//---------------------------------------------------------
paxos_instance::paxos_instance(std::shared_ptr<Node> network_uid, std::vector<std::shared_ptr<Node>> peers, std::string filename, void(*callback)(void*, size_t)){
    PRINTF("paxos_instance::paxos_instance\n");
    _state_file         = filename;
    _peers              = peers;
    _quorum_size        = (_peers.size() / 2) + 1;
    _network_uid        = std::make_shared<Node>(network_uid->get_ip(), network_uid->get_port());
    
    
    _completed_value_callback = callback;
    
    _instance_number    = 0;
    _promised_id        = NULL;
    _accepted_id        = NULL;
    _accepted_value     = NULL;
    _highest_proposal_id    = std::make_shared<PAXOS_ID>(0, _network_uid);;
    _highest_accepted_id    = NULL;
    _current_prepare_message     = NULL;
    _current_accept_message = NULL;
    _promises_received.clear();
    _nacks_received.clear();
    
    _proposals.clear();
    _acceptors.clear();
    _final_value        = NULL;
    _final_acceptors.clear();
    _final_proposal_id  = NULL;
    
    _proposed_value     = NULL;
    _leader             = false;
    _proposal_id        = std::make_shared<PAXOS_ID>(0, _network_uid);
    
    _current_value      = NULL;
    
    //_proposer = new proposer(_network_uid, _quorum_size);
    //_acceptor = new acceptor(_network_uid, NULL, NULL, NULL);
    //_learner  = new learner(_network_uid, _quorum_size);
    
    // for the master lease
    _master_uid = NULL;
    _master_attempt = false;
    _initial_load   = true;
    _lease_window   = 1000;
    _lease_timer_id = TimerThread::no_timer;
    
    // for the exp back resolution
    // time in millisecs
#if 1
    _backoff_inital         = 5;//10000;
    _backoff_cap            = 2000;//10000;
    _drive_silence_timeout  = 3000;//10000;
    _retransmit_interval    = 1000;//10000;
    _backoff_window         = _backoff_inital;
    _retransmit_timer       = TimerThread::no_timer;
    _delayed_drive          = TimerThread::no_timer;
    
    //For the sync
    _sync_delay             = 100;//10000.0;
#else
    _backoff_inital         = 10000;
    _backoff_cap            = 10000;
    _drive_silence_timeout  = 10000;
    _retransmit_interval    = 10000;
    _backoff_window         = _backoff_inital;
    _retransmit_timer       = TimerThread::no_timer;
    _delayed_drive          = TimerThread::no_timer;
    
    //For the sync
    _sync_delay             = 10000.0;
#endif
    
}
//---------------------------------------------------------
paxos_instance::~paxos_instance(){
    TODO("pacxos_instance: delete all the things\n");
}
//---------------------------------------------------------
void paxos_instance::start(){
      
    _sync_timer.addTimer(0, _sync_delay, [=](){send_sync(_peers.at(rand() % _peers.size()) , _instance_number);});
    
    load_state();
}
//---------------------------------------------------------
void paxos_instance::save_state(size_t instance_number, 
        std::shared_ptr<Value> current_value,
        std::shared_ptr<PAXOS_ID> promised_id, 
        std::shared_ptr<PAXOS_ID> accetped_id, 
        std::shared_ptr<Value> accepted_value){
    std::ofstream file;
    size_t len = 0, pos = 0;
    char* buf;
    
    PRINTF("paxos_instance::save_state\n");
    _instance_number	= instance_number;
    _current_value	= current_value;
    _promised_id	= promised_id;
    _accepted_id	= accetped_id;
    _accepted_value	= accepted_value;

    std::string tmp = _state_file + "tmp";

    file.open(tmp);
    if(!file){
            ERROR("LOAD_STATE: problem loading state file.\n");
    }
    
    len += sizeof(size_t);
    len += sizeof(size_t);
    len += Value::marshal(_current_value, NULL, 0, false);
    len += PAXOS_ID::marshal(_promised_id, NULL, 0, false);
    len += PAXOS_ID::marshal(_accepted_id, NULL, 0, false);
    len += Value::marshal(_accepted_value, NULL, 0, false);
    
    buf = (char*)malloc(len);
    
    memcpy(buf, &len, sizeof(size_t));
    pos += sizeof(size_t);
    memcpy(buf + pos, &_instance_number, sizeof(size_t));
    pos += sizeof(size_t);
    pos += Value::marshal(_current_value, buf + pos, len, true);
    pos += PAXOS_ID::marshal(_promised_id, buf + pos, len, true);
    pos += PAXOS_ID::marshal(_accepted_id, buf + pos, len, true);
    pos += Value::marshal(_accepted_value, buf + pos, len, true);
    
    file.write(buf, len);
    file.flush();
    file.close();
    
    free(buf);
    
    PRINTF("SAVE STATE: _instance_num %ld\n",   _instance_number);
    PRINTF("SAVE STATE: _current_value %s\n",   (_current_value)?_current_value->to_str().c_str():"NULL");
    PRINTF("SAVE STATE: _promised_id %s\n",    (_promised_id)?_promised_id->to_str().c_str():"NULL");
    PRINTF("SAVE STATE: _accepted_id %s\n",    (_accepted_id)?_accepted_id->to_str().c_str():"NULL");
    PRINTF("SAVE STATE: _accepted_value %s\n", (_accepted_value)?_accepted_value->to_str().c_str():"NULL");

    // rename() is an atomic filesystem operation. By writing the new
    // state to a temporary file and using this method, we avoid the potential
    // for leaving a corrupted state file in the event that a crash/power loss
    // occurs in the middle of update.
    if(rename((_state_file + "tmp").c_str(), _state_file.c_str()) != 0){
        ERROR("SAVE_STATE: there was a problem renaming the file\n");
    }
}
//---------------------------------------------------------
void paxos_instance::load_state(){
    std::fstream file;
    size_t len = 0, pos = 0, tmp_pos;
    char* buf;
    
    file.open(_state_file, std::ios::out | std::ios::in);
    
    if(!file){
        // file does not exit, make it
        len += sizeof(size_t);                              //total encode
        len += sizeof(size_t);                              //instance num
        len += Value::marshal(_current_value, NULL, 0, false);
        len += PAXOS_ID::marshal(_promised_id, NULL, 0, false);
        len += PAXOS_ID::marshal(_accepted_id, NULL, 0, false);
        len += Value::marshal(_accepted_value, NULL, 0, false);

        buf = (char*)malloc(len);

        memcpy(buf, &len, sizeof(size_t));
        pos += sizeof(size_t);
        memcpy(buf + pos, &_instance_number, sizeof(size_t));
        pos += sizeof(size_t);
        pos += Value::marshal(_current_value, buf + pos, len, true);
        pos += PAXOS_ID::marshal(_promised_id, buf + pos, len, true);
        pos += PAXOS_ID::marshal(_accepted_id, buf + pos, len, true);
        pos += Value::marshal(_accepted_value, buf + pos, len, true);

        file.write(buf, len);
    }
    else{
        file.read((char*)&len, sizeof(size_t));
        file.read((char*)&_instance_number, sizeof(size_t));
        
        buf = (char*) malloc(len);
        
        // skip length and instance num
        file.read(buf, len - (2*sizeof(size_t)));
        
        tmp_pos = 0;
        _current_value = Value::demarshal(buf, len, &tmp_pos);
        pos += tmp_pos;
        
        tmp_pos = 0;
        _promised_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
        pos += tmp_pos;
        
        tmp_pos = 0;
        _accepted_id = PAXOS_ID::demarshal(buf + pos, &tmp_pos);
        pos += tmp_pos;
        
        tmp_pos = 0;
        _accepted_value = Value::demarshal(buf + pos, len, &tmp_pos);
        pos += tmp_pos;
        
        
    }

    file.flush();
    file.close();
    free(buf);
    
    PRINTF("LOAD STATE: _instance_num %ld\n",   _instance_number);
    PRINTF("LOAD STATE: _current_value %s\n",   (_current_value)?_current_value->to_str().c_str():"NULL");
    PRINTF("LOAD STATE: _promised_id %s\n",    (_promised_id)?_promised_id->to_str().c_str():"NULL");
    PRINTF("LOAD STATE: _accepted_id %s\n",    (_accepted_id)?_accepted_id->to_str().c_str():"NULL");
    PRINTF("LOAD STATE: _accepted_value %s\n", (_accepted_value)?_accepted_value->to_str().c_str():"NULL");
    
    // MASTER CODE
    if(_initial_load){
        _initial_load = false;
        update_lease(NULL);
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
void paxos_instance::receive_prepare(PAXOS_MSG msg){
    PRINTF("paxos_instance::receive_prepare :: FROM %s\n", msg.get_from_uid()->to_str().c_str());
    proposer_observe_proposal(msg.get_proposal_id());
    
    //printf("%p\n", _master_uid);
    //printf("%p\n", _instance_number);
    //printf("%p\n", _master_uid);
    
    // drop non-master requests and messages not from the current link in the 
    // multi paxos chain
    if((_master_uid != NULL && _master_uid->cmp_neq(msg.get_from_uid()))
            || _instance_number != msg.get_instance_number()){
        return;
    }

    std::shared_ptr<PAXOS_MSG> reply = acceptor_receive_prepare(msg);
    if(reply->get_type() == promise_enum){
        save_state(_instance_number, _current_value, reply->get_proposal_id(), reply->get_last_accepted_id(), reply->get_last_accepted_value());
        send_promise(msg.get_from_uid(), _instance_number, reply->get_proposal_id(), reply->get_last_accepted_id(), reply->get_last_accepted_value());
    }
    else{
        send_nack(msg.get_from_uid(), _instance_number, msg.get_proposal_id(), _promised_id);
    }
    //delete reply;
}
//---------------------------------------------------------
void paxos_instance::receive_accept(PAXOS_MSG msg){
    proposer_observe_proposal(msg.get_proposal_id());
    
    // drop non-master requests and messages not from the current link in the 
    // multi paxos chain
    if((_master_uid != NULL && _master_uid->cmp_neq(msg.get_from_uid()))
            || _instance_number != msg.get_instance_number()){
        return;
    }
    
    std::shared_ptr<PAXOS_MSG> reply = acceptor_receive_accept(msg);
    if(reply->get_type() == accepted_enum){
        save_state(_instance_number, _current_value, _promised_id, msg.get_proposal_id(), msg.get_proposal_value());
        send_accepted(_instance_number, reply->get_proposal_id(), reply->get_proposal_value());
    }
    else{
        send_nack(msg.get_from_uid(), _instance_number, msg.get_proposal_id(), _promised_id);
    }
    //delete reply;
    
    // The peer proposing the value could fail before resolution is achieved. Step in to complete the process if
    // the drive_silence_timeout elapses with no messages received
    reschedule_next_drive_attempt(_drive_silence_timeout / 1000);
}
//---------------------------------------------------------
void paxos_instance::receive_nack(PAXOS_MSG msg){
    PRINTF("paxos_instance::receive_nack\n");
    if(_instance_number != msg.get_instance_number()){
        //only process the messages for current link in multi paxos
        return;
    }
    
    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<double> unif(0, 1);
    
    proposer_receive_nack(msg);
    
    stop_driving();
    
    _backoff_window = _backoff_window * 2;
    if(_backoff_window > _backoff_cap){
        _backoff_window = _backoff_cap;
    }
    reschedule_next_drive_attempt((_backoff_window * unif(gen))); 
    
}
//---------------------------------------------------------
void paxos_instance::receive_promise(PAXOS_MSG msg){
    PRINTF("paxos_instance::receive_promise\n");
    if(_instance_number != msg.get_instance_number()){
        //only process the messages for current link in multi paxos
        return;
    }
    
    std::shared_ptr<PAXOS_MSG> reply = proposer_receive_promise(msg);
    if(reply != NULL){
        send_accept(reply->get_proposal_id(), reply->get_proposal_value());
    }
    
    // NB: the reply message memory is managed by receive promise
}
//---------------------------------------------------------
void paxos_instance::receive_accepted(PAXOS_MSG msg){
    PRINTF("paxos_instance::receive_accepted\n");
    if(_instance_number != msg.get_instance_number()){
        //only process the messages for current link in multi paxos
        return;
    }
    
    std::shared_ptr<PAXOS_MSG> reply = learner_receive_accepted(msg);
    if(reply != NULL){
        advance_instance(_instance_number + 1, msg.get_proposal_value(), false);
    }
    
}
//---------------------------------------------------------
//---------------------------------------------------------
void paxos_instance::send_accept_REAL(std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> proposal_value){
    PRINTF("paxos_instance::send_accept_REAL\n");
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::accept(_network_uid, _instance_number, proposal_id, proposal_value);
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    for(std::shared_ptr<Node> i : _peers){
        c = new UDP_comms(i->get_ip(), i->get_port(), false);
        c->send(buf, buf_len);
        delete c;
    }
    
    free(buf);
    //delete msg;
}
//---------------------------------------------------------
void paxos_instance::send_accept(std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> proposal_value){
    if(_timer.exists(_retransmit_timer)){
        _timer.clearTimer(_retransmit_timer);
    }
    _retransmit_timer = _timer.addTimer(0, _retransmit_interval, [=](){send_accept_REAL(proposal_id, proposal_value);});
}
//---------------------------------------------------------
void paxos_instance::send_accepted(size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> value){
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::accepted(_network_uid, _instance_number, proposal_id, value);
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    for(std::shared_ptr<Node> i : _peers){
        c = new UDP_comms(i->get_ip(), i->get_port(), false);
        c->send(buf, buf_len);
        delete c;
    }
    
    free(buf);
    //delete msg;
}
//---------------------------------------------------------
void paxos_instance::send_promise(std::shared_ptr<Node> dest, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> last_accepted_id, std::shared_ptr<Value> last_accepted_value){
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::promise(_network_uid, dest, instance_num, proposal_id, last_accepted_id, last_accepted_value);
    
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    c = new UDP_comms(dest->get_ip(), dest->get_port(), false);
    c->send(buf, buf_len);
        
    free(buf);
    delete c;
    //delete msg;
    
}
//---------------------------------------------------------
void paxos_instance::send_nack(std::shared_ptr<Node> dest, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> promised_id){
    PRINTF("paxos_instance::send_nack\n");
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::nack(_network_uid, dest, instance_num, proposal_id, promised_id);
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    c = new UDP_comms(dest->get_ip(), dest->get_port(), false);
    c->send(buf, buf_len);
    delete c;
    
    free(buf);
    //delete msg;
}
//---------------------------------------------------------
void paxos_instance::send_prepare(std::shared_ptr<PAXOS_ID> id){
    PRINTF("paxos_instance::send_prepare :: %s\n", id->to_str().c_str());
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::prepare(_network_uid, _instance_number, id);
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    //for(int i = 0 ; i < buf_len ; i++){
    //    printf("[%d] %c (%02X)\n", i, *(buf + i), *(buf + i));
    //}
    
    for(std::shared_ptr<Node> i : _peers){
        c = new UDP_comms(i->get_ip(), i->get_port(), false);
        c->send(buf, buf_len);
        delete c;
    }
    
    free(buf);
    //delete msg;
}
//---------------------------------------------------------
void paxos_instance::send_sync(std::shared_ptr<Node> dest, size_t instance_number){
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::sync(_network_uid, instance_number);
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    c = new UDP_comms(dest->get_ip(), dest->get_port(), false);
    c->send(buf, buf_len);
        
    free(buf);
    delete c;
    //delete msg;
}
//---------------------------------------------------------
void paxos_instance::send_catchup(std::shared_ptr<Node> dest, size_t instance_num, std::shared_ptr<Value> value){
    UDP_comms* c;
    char* buf;
    size_t buf_len;
    std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::catchup(dest, instance_num, value);
    
    buf_len = msg->marshal_len();
    buf     = (char*)malloc(buf_len);
    msg->marshal(buf, buf_len);
    
    c = new UDP_comms(dest->get_ip(), dest->get_port(), false);
    c->send(buf, buf_len);
        
    free(buf);
    delete c;
    //delete msg;
}
//---------------------------------------------------------
//---------------------------------------------------------
void paxos_instance::propose_value(std::shared_ptr<Value> v){
    proposer_propose_value(v);
}
//---------------------------------------------------------
//---------------------------------------------------------
int paxos_instance::handle_propose(PAXOS_MSG msg){
    PRINTF("handle_propose\n");
    return propose_update(msg.get_proposal_value(), true);
}
//---------------------------------------------------------
void paxos_instance::handle_prepare(PAXOS_MSG msg){
    //PRINTF("handle_prepare\n");
    
    receive_prepare(msg);
}
//---------------------------------------------------------
void paxos_instance::handle_promise(PAXOS_MSG msg){
    //PRINTF("handle_promise\n");
    receive_promise(msg);
}
//---------------------------------------------------------
void paxos_instance::handle_accpt_req(PAXOS_MSG msg){
    //PRINTF("handle_accpt_req\n");
    //observe_proposal();
    
    receive_accept(msg);
    //_acceptor->receive_accept(_peers, msg);
}
//---------------------------------------------------------
void paxos_instance::handle_accpted(PAXOS_MSG msg){
    //PRINTF("handle_accpted\n");
    receive_accepted(msg);
    //_learner->receive_accepted(msg);
}
//---------------------------------------------------------
void paxos_instance::handle_nack(PAXOS_MSG msg){
    //PRINTF("handle_nack\n");
    receive_nack(msg);
}
//---------------------------------------------------------
void paxos_instance::handle_catchup(PAXOS_MSG msg){
    //PRINTF("handle_catchup\n");
    receive_catchup(msg.get_from_uid(), msg.get_instance_number(), msg.get_proposal_value());
}
//---------------------------------------------------------
void paxos_instance::handle_sync(PAXOS_MSG msg){
    //PRINTF("handle_sync_request\n");
    //PRINTF("handle_sync_request : %ld || %ld\n", msg.get_from_uid()->to_str().c_str(), msg.get_instance_number());
    receive_sync_request(msg.get_from_uid(), msg.get_instance_number());
}
//---------------------------------------------------------
//---------------------------------------------------------
#define MAX_BUFFER  10240
void paxos_instance::daemon(Node n){
    TODO("register a handler to catch sigkill to close things down nicely\n");
    
    char buffer[MAX_BUFFER];
    UDP_comms *c = new UDP_comms(n.get_ip(), n.get_port(), true);
    size_t read_bytes;
    
    PRINTF("paxos_instance::daemon :: listening on %s,%d\n", n.get_ip().c_str(), n.get_port());
    
    while(1){
        read_bytes = c->recv(buffer, MAX_BUFFER);
        
        if(read_bytes > MAX_BUFFER){
            TODO("there is more to read\n");
        }
        
        //PRINTF("DAEMON: read %ld bytes\n", read_bytes);
        //for(auto i = 0 ; i < read_bytes ; i++){
        //    printf("[%d] %c\n", i, *(buffer + i));    
        //}
        
        //MSG_ENUM_TYPE tmp_type = (MSG_ENUM_TYPE)*buffer;
        std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::demarshal(buffer, read_bytes);
        
        // now dispatch the message
        switch(msg->get_type()){
            case propose_enum:
                handle_propose(*msg);
                break;
            case prepare_enum:
                handle_prepare(*msg);
                break;
            case promise_enum:
                handle_promise(*msg);
                break;
            case accept_req_enum:
                handle_accpt_req(*msg);
                break;
            case accepted_enum:
                handle_accpted(*msg);
                break;
            case permission_nack_enum:
                // look at what type of nack is ocming back and act accordingly
                PRINTF("PERMISSION_NACK: %s\n", msg->get_from_uid()->to_str().c_str());
                break;
            case suggestion_nack_enum:
                // look at what type of nack is ocming back and act accordingly
                PRINTF("SUGGESTION_NACK: %s\n", msg->get_from_uid()->to_str().c_str());
                break;
            case accepted_nack_enum:
                // look at what type of nack is ocming back and act accordingly
                PRINTF("ACCEPTED_NACK: %s\n", msg->get_from_uid()->to_str().c_str());
                break;
            case nack_enum:
                handle_nack(*msg);
                break;
            case null_enum:
                PRINTF("NULL: should never see this...\n");
                break;
            case catchup_enum:
                handle_catchup(*msg);
                break;
            case sync_enum:
                handle_sync(*msg);
                break;
            case resolution_enum:
                TODO("resolution_enum\n");
                break;
        }
        //delete msg;
    }
}
//---------------------------------------------------------
void paxos_instance::client_daemon(Node n){
    TODO("register a handler to catch sigkill to close things down nicely\n");
    
    char buffer[MAX_BUFFER];
    TCP_comms *c = new TCP_comms(n.get_ip(), n.get_port(), true);
    size_t read_bytes;
    int client_sock;
    bool waiting = true;
    
    char ack = '1';
    
    // this should be malloc'd on demand
    size_t  buf_len;
    char*   buf;
    
    
    PRINTF("paxos_instance::client_daemon :: listening on %s,%d\n", n.get_ip().c_str(), n.get_port());
    
    while(1){
        
        if(waiting){
            PRINTF("paxos_instance::client_daemon - try and accept incomming requests\n");
            client_sock = c->accept();
            waiting = false;
        }
        else{
            
            PRINTF("paxos_instance::client_daemon - read some data\n");
            read_bytes = c->recv(client_sock, buffer, MAX_BUFFER);

            if(read_bytes > MAX_BUFFER){
                TODO("there is more to read\n");
            }
            else{
                waiting = true;
            }

            //PRINTF("DAEMON: read %ld bytes\n", read_bytes);
            //for(auto i = 0 ; i < read_bytes ; i++){
            //    printf("[%d] %c\n", i, *(buffer + i));    
            //}

            //MSG_ENUM_TYPE tmp_type = (MSG_ENUM_TYPE)*buffer;
            std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::demarshal(buffer, read_bytes);

            // now dispatch the message
            switch(msg->get_type()){
                case propose_enum:
                    // try and propose
                    if(!handle_propose(*msg)){
                        
                        TODO("either we are not the master, or we are doing a proposal\n");
                        buf_len = Node::marshal(_master_uid, NULL, 0, false);
                        buf     = (char*)malloc(buf_len);
                        Node::marshal(_master_uid, buf, buf_len, true);
                        
                        c->send(client_sock, buf, buf_len);
                        
                        free(buf);
                    }
                    else{ // send the current master node
                        c->send(client_sock, &ack, sizeof(char));
                    }
                    close(client_sock);
                    break;
                default:
                    TODO("why is a client sending anything but a request on tcp\n");
            }
        }
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
// FOR THE MASTER LEASE CODE
void paxos_instance::start_master_lease_timer(){
    PRINTF("paxos_instance::start_master_lease_timer\n");
    if(_timer.exists(_lease_timer_id)){
        _timer.clearTimer(_lease_timer_id);
    }
    
    // starting from now
    _lease_timer_id  = _timer.setTimeout([=](){lease_expired();}, _lease_window);
}
//---------------------------------------------------------
void paxos_instance::update_lease(std::shared_ptr<Node> master_uid){
    PRINTF("paxos_instance::update_lease\n");
    _master_uid = master_uid;
    
    if(_master_uid == NULL || _network_uid->cmp_neq(master_uid)){
        start_master_lease_timer();
    }
    else{
        
        if(_timer.exists(_lease_timer_id))
        {
            /*
            _timer.clearTimer(_lease_timer_id);
            
            TODO("A - THIS NEEDS TO SEND THE NODE FOR THE NEW MASTER\n");
            _lease_timer_id  = _timer.setTimeout([=](){
                    std::shared_ptr<Value> v = std::make_shared<Value>(true, _network_uid, "", 0);
                    propose_update(v, false);
                    //delete v;
                },  _lease_window);
             */ 
        }
        else{
            TODO("B - THIS NEEDS TO SEND THE NODE FOR THE NEW MASTER\n");
            std::shared_ptr<Value> v = std::make_shared<Value>(true, _network_uid, nullptr, 0);
            propose_update(v, false);
            //delete v;
        }
    }
}
//---------------------------------------------------------
void paxos_instance::lease_expired(){
    _master_uid = NULL;
    std::shared_ptr<Value> v = std::make_shared<Value>(true, _network_uid, nullptr, 0);
    propose_update(v, false);
    //delete v;
}
//---------------------------------------------------------
int paxos_instance::propose_update(std::shared_ptr<Value> v, bool application_level){
    PRINTF("paxos_instance::propose_update : %s\n", application_level?"APP":"MASTER");
    if(application_level){
        if(_master_uid != NULL && _master_uid->cmp_eq(_network_uid)){
            // proposer update
            propose_value(v);
            drive_to_resolution();
        }
        else{
            PRINTF("PAXOS_INSTANCE:: ignoring proposal as not master\n");
            TODO("paxos_instance::propose_update: add a nack to say where to try"
                    "or forward to the right guy\n");
            return 0;
        }
    }
    else{
        if((_master_uid == NULL || _master_uid->cmp_eq(_network_uid)) && !_master_attempt){
          _master_attempt = true;
          start_master_lease_timer();
          
          TODO("propose_update the master attempt\n");
          propose_value(v);
          drive_to_resolution();
        }
    }
    return 1;
}
//---------------------------------------------------------
void paxos_instance::advance_instance(size_t new_instance_num, std::shared_ptr<Value> new_current_value, bool catchup){
    PRINTF("paxos_instance::advance_instance\n");
    _master_attempt = false;
    if(catchup){
        //RESOLUTION
        save_state(new_instance_num, new_current_value, NULL, NULL, NULL);
        
        //_instance_number    = new_instance_num;
        //_current_value      = new_current_value;
        _promised_id        = NULL;
        _accepted_id        = NULL;
        _accepted_value     = NULL;
        _proposed_value     = NULL;
        
        PRINTF("CATCHUP_UPDATE inst_num = %ld, value = %s\n", new_instance_num, VALUE_STR(new_current_value));
        stop_driving();
        _backoff_window = _backoff_inital;
        
        proposer_prepare();
        return;
    }
    
    //get the value
    if(new_current_value->is_master()){
        PRINTF("paxos_instance::advance_instance: NEW MASTER LEASE %s\n", VALUE_STR(new_current_value->get_master()));
        
        update_lease(new_current_value->get_master());
        
        TODO("????\n");
        //new_current_value = _current_value;
    }
    else{
        PRINTF("paxos_instance::advance_instance: APP VALUE\n");
        //PRINTF("paxos_instance::advance_instance: APP VALUE %s\n", new_current_value->get_value().c_str());
        //new_current_value = tmp_val;
        
        //TODO("paxos_instance::advance_instance: have a new value, now callback\n");
        _completed_value_callback(new_current_value->get_value(), new_current_value->get_len());
    }
    
    save_state(new_instance_num, new_current_value, NULL, NULL, NULL);
        
    //_instance_number    = new_instance_num;
    //_current_value      = new_current_value;
    _promised_id        = NULL;
    _accepted_id        = NULL;
    _accepted_value     = NULL;
    _proposed_value     = NULL;

    PRINTF("UPDATE inst_num = %ld, %s\n", new_instance_num, VALUE_STR(new_current_value));
    
    stop_driving();
    _backoff_window = _backoff_inital;
    
    if(_master_uid != NULL){
        std::shared_ptr<PAXOS_ID> master = std::make_shared<PAXOS_ID>(1, _master_uid);
        if(_master_uid->cmp_eq(_network_uid)){
            PRINTF("paxos_instance::advance_instance :: MASTER\n");
            proposer_prepare();
            std::shared_ptr<PAXOS_MSG> msg;
            for(auto i : _peers){
                msg = PAXOS_MSG::promise(i, _network_uid, _instance_number, master, NULL, NULL);
                proposer_receive_promise(*msg);
                //delete msg;
            }
        }
        else{
            PRINTF("paxos_instance::advance_instance :: still not master\n");
            
            std::shared_ptr<PAXOS_MSG> msg = PAXOS_MSG::prepare(_master_uid, _instance_number, master);
            acceptor_receive_prepare(*msg);
            proposer_observe_proposal(master);
            //delete msg;
        }
        //delete master;
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
// FOR THE SUNC REQUESTS
/*
void paxos_instance::sync(){
    TODO("\n");
    TODO("make paxos sync message with instance number\n");
    //send sync_request(rand(_peers), _instance_number);
    
    TODO("something should setup a timer to keep calling this function\n");
}
*/
//---------------------------------------------------------
void paxos_instance::receive_sync_request(std::shared_ptr<Node> from, size_t instance_number){
    //PRINTF("paxos_instance::receive_sync_request\n");
    //PRINTF("paxos_instance::receive_sync_request : s\n", from->to_str().c_str());
    //PRINTF("paxos_instance::receive_sync_request : %ld || %ld\n", instance_number, _instance_number);
    if(instance_number < _instance_number){
        //PRINTF("paxos_instance::receive_sync_request : SEND CATCHUP\n");
        send_catchup(from, _instance_number, _current_value);
    }
#if DEBUG
    else{
      //PRINTF("paxos_instance::receive_sync_request : NOTHING TO SYNC\n");
    }
#endif
}
//---------------------------------------------------------
void paxos_instance::receive_catchup(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<Value> v){
    PRINTF("paxos_instance::receive_catchup :: %ld || %ld\n", instance_number, _instance_number);
    if(instance_number > _instance_number){
        PRINTF("SYNCHRONISED :: %ld, %s\n", instance_number, VALUE_STR(v));
        advance_instance(instance_number, v, true);
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
// for the drive to resolution
void paxos_instance::reschedule_next_drive_attempt(size_t delay){
    PRINTF("paxos_instance::reschedule_next_drive_attempt :: delay %ld \n", delay);
    if(_timer.exists(_delayed_drive)){
        _timer.clearTimer(_delayed_drive);
    }
    _delayed_drive = _timer.addTimer(delay, 0, [=](){drive_to_resolution();});
}
//---------------------------------------------------------
void paxos_instance::drive_to_resolution(){
    PRINTF("paxos_instance::drive_to_resolution\n");
    if(_master_uid != NULL && _master_uid->cmp_eq(_network_uid)){
        stop_driving();
        if(_proposal_id->get_count() == 1){
            send_accept(_proposal_id, _proposed_value);
        }
        else{
            proposer_prepare();
        }
        _retransmit_timer = _timer.addTimer(0, _retransmit_interval, [=](){send_prepare(_proposal_id);});
    }
    else{
        stop_driving();
        std::shared_ptr<PAXOS_MSG> msg = proposer_prepare();
        _retransmit_timer = _timer.addTimer(0, _retransmit_interval, [=](){send_prepare(_proposal_id);});
    }
}
//---------------------------------------------------------
void paxos_instance::stop_driving(){
    PRINTF("paxos_instance::stop_driving\n");
    if(_timer.exists(_retransmit_timer)){
        _timer.clearTimer(_retransmit_timer);
    }
    
    if(_timer.exists(_delayed_drive)){
        _timer.clearTimer(_delayed_drive);
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//------------------------------------------------------------------------------
// The acceptor functions
//------------------------------------------------------------------------------
std::shared_ptr<PAXOS_MSG> paxos_instance::acceptor_receive_accept(PAXOS_MSG msg){
    std::shared_ptr<PAXOS_MSG> reply;
    
    if(msg.get_proposal_id()->cmp_geq(_promised_id)){
        _promised_id 	= msg.get_proposal_id();
        _accepted_id 	= msg.get_proposal_id();
        _accepted_value	= msg.get_proposal_value();

        PRINTF("acceptor::receive_accept: reply accepted\n");
        reply = PAXOS_MSG::accepted(_network_uid, _instance_number, msg.get_proposal_id(), msg.get_proposal_value());
    }
    else{
        reply = PAXOS_MSG::nack(_network_uid, msg.get_from_uid(), _instance_number, msg.get_proposal_id(), _promised_id);
    }
    return reply;
}
//------------------------------------------------------------------------------
std::shared_ptr<PAXOS_MSG> paxos_instance::acceptor_receive_prepare(PAXOS_MSG msg){
    PRINTF("acceptor::receive_prepare\n");
    std::shared_ptr<PAXOS_MSG> reply;
    
    proposer_observe_proposal(msg.get_proposal_id());
    
    if(_promised_id == NULL || msg.get_proposal_id()->cmp_geq(_promised_id)){
        PRINTF("acceptor:: newer msg\n");
        _promised_id = msg.get_proposal_id();
        reply = PAXOS_MSG::promise(_network_uid, msg.get_from_uid(), _instance_number, _promised_id, _accepted_id, _accepted_value);
    }
    else{
        PRINTF("acceptor:: old msg\n");
        reply =  PAXOS_MSG::nack(_network_uid, msg.get_from_uid(), _instance_number, msg.get_proposal_id(), _promised_id);
    }
    
    return reply;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// The Learner Functions
#include <cassert>
//------------------------------------------------------------------------------
std::shared_ptr<PAXOS_MSG> paxos_instance::learner_receive_accepted(PAXOS_MSG msg){
    PROPOSAL_STATUS* ps;
    std::shared_ptr<PAXOS_ID> last_pn = NULL;

    if(_final_value != NULL){
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
           delete ps;
           _proposals.erase(last_pn->to_str());
       }
    }

    // if a new proposal id, create a new record
    if(_proposals.count(msg.get_proposal_id()->to_str()) == 0){
        PROPOSAL_STATUS* tmp_ps = new PROPOSAL_STATUS;
        tmp_ps->accept_count = 0;
        tmp_ps->retain_count = 0;
        tmp_ps->v            = msg.get_proposal_value();
        tmp_ps->acceptors.clear();

        _proposals[msg.get_proposal_id()->to_str()] = tmp_ps;
    }

    ps = _proposals[msg.get_proposal_id()->to_str()];
    //PRINTF("learner::receive_accepted : proposed %p || current %p\n", msg.get_proposal_value(), ps->v);
    //PRINTF("learner::receive_accepted : proposed %s || current %s\n", msg.get_proposal_value()->get_value().c_str(), ps->v->get_value().c_str());
    //TODO("learner::receive_accepted : make the value assertion generic || %d\n", msg.get_proposal_value()->cmp_eq( ps->v));
    //assert(!msg.get_proposal_value()->cmp_eq( ps->v));

    ps->accept_count++;
    ps->retain_count++;
    ps->acceptors.insert(std::pair<std::string, std::shared_ptr<PAXOS_ID>>(msg.get_from_uid()->to_str(),  nullptr));

    if(ps->accept_count == _quorum_size){
        _final_proposal_id  = msg.get_proposal_id();
        _final_value        = msg.get_proposal_value();
        _final_acceptors    = ps->acceptors;

        std::map<std::string, PROPOSAL_STATUS*>::iterator itr = _proposals.begin();
        while (itr != _proposals.end()) {
               std::map<std::string, PROPOSAL_STATUS*>::iterator toErase = itr;
               ++itr;
               delete toErase->second;
               _proposals.erase(toErase);
        }
        _proposals.clear();
        _acceptors.clear();

        //TODO("learner::receive_accepted - FINAL VALUE : %s\n", _final_value->get_value().c_str());
        TODO("learner::receive_accepted - FINAL VALUE : <SOMETHING>\n");
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
// the proposer functions
//------------------------------------------------------------------------------
void paxos_instance::proposer_propose_value(std::shared_ptr<Value> v){
    PRINTF("proposer::propose_value\n");
    std::shared_ptr<PAXOS_MSG> m;
    size_t len;
    char* tmp_buf;
    UDP_comms *c;
    
    if(_proposed_value == NULL){
        _proposed_value = v;
        
        if(_leader){
            PRINTF("PROPOSER: PROPOSE_VALUE: we are leader, auto accept\n");
            
            //delete _current_accept_message;
            _current_accept_message = PAXOS_MSG::accept(_network_uid, _instance_number, _proposal_id, v);
        }
        else{
            PRINTF("PROPOSER: PROPOSE_VALUE: not leader, propose\n");
            m       = PAXOS_MSG::prepare(_network_uid, _instance_number, _highest_proposal_id);
            len     = m->marshal_len();
            tmp_buf = (char*) malloc(len);
            memset(tmp_buf, 0, len);
            m->marshal(tmp_buf, len);
            
            //for(int i = 0 ; i < len ; i++){
            //    printf("[%d] %c (%02X)\n", i, *(tmp_buf + i), *(tmp_buf + i));
            //}
            
            for(auto i : _peers){
                PRINTF("PROPOSER: %s:%d\n", i->get_ip().c_str(), i->get_port());
                c = new UDP_comms(i->get_ip(), i->get_port(), false);
                
                c->send(tmp_buf, len);
                delete c;
            }
            
            free(tmp_buf);
            //delete m;
        }
    }
}
//------------------------------------------------------------------------------
std::shared_ptr<PAXOS_MSG> paxos_instance::proposer_prepare(){
	_leader         		= false;
        _promises_received.clear();
	_nacks_received.clear();
        _proposal_id 			= std::make_shared<PAXOS_ID>(_highest_proposal_id->get_count() + 1, _network_uid);
        
        //delete _highest_proposal_id;
	_highest_proposal_id 		= _proposal_id;
	_current_prepare_message 	= PAXOS_MSG::prepare(_network_uid, _instance_number, _proposal_id);
	
        PRINTF("proposer::prepare : new proposal_id = %s\n", _proposal_id->to_str().c_str());
        return _current_prepare_message;
}
//------------------------------------------------------------------------------
void paxos_instance::proposer_observe_proposal(std::shared_ptr<PAXOS_ID> proposal_id){
    if(proposal_id > _highest_proposal_id){
        _highest_proposal_id = proposal_id;
    }
}
//------------------------------------------------------------------------------
void paxos_instance::proposer_receive_nack(PAXOS_MSG msg){
    proposer_observe_proposal(msg.get_promised_proposal_id());

    if(msg.get_proposal_id()->cmp_eq(_proposal_id) && !_nacks_received.empty()){

        TODO("proposer::receive_nack - Should make a copy of this \n");
        _nacks_received.insert(msg.get_from_uid()->to_str());

        if(_nacks_received.size() == _quorum_size){
            // lost the leadership or failed to acquire it
            TODO("proposer::receive_nack - prepare??\n");
            std::shared_ptr<PAXOS_MSG> tmp = proposer_prepare();
            //delete tmp;
        }
    }
}
//------------------------------------------------------------------------------
/**
 * Returns an Accept messages if a quorum of Promise messages is achieved
 * @param msg
 * @return 
 */
std::shared_ptr<PAXOS_MSG> paxos_instance::proposer_receive_promise(PAXOS_MSG msg){
    proposer_observe_proposal(msg.get_proposal_id());

    PRINTF("proposer::receive_promise\n");
    if(!_leader 
        && msg.get_proposal_id()->cmp_eq(_proposal_id)
        && _promises_received.count(msg.get_from_uid()) == 0){

        _promises_received.insert(msg.get_from_uid());

        if(msg.get_last_accepted_id() != NULL && msg.get_last_accepted_id()->cmp_gt(_highest_accepted_id)){
            _highest_accepted_id = msg.get_last_accepted_id();

            if(msg.get_last_accepted_value() != NULL){
                _proposed_value = msg.get_last_accepted_value();
            }
        }

        if(_promises_received.size() == _quorum_size){
            _leader = true;
            // we now have consensus, as the group
            if(_proposed_value != NULL)
            {
                //delete _current_accept_message;
                _current_accept_message = PAXOS_MSG::accept(_network_uid, _instance_number, _proposal_id, _proposed_value);
                return _current_accept_message;
            }
        }
    }
    return NULL;
}
//------------------------------------------------------------------------------
void paxos_instance::set_update_value_callback(void(*callback)(void*, size_t)){
    _completed_value_callback = callback;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
