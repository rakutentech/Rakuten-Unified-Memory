#ifndef __PAXOS_INSTANCE__
#define __PAXOS_INSTANCE__

#include "util.h"

#include "paxos_msg.h"
#include "paxos_id.h"
#include "acceptor.h"
#include "learner.h"
#include "proposer.h"
#include "node.h"
#include "paxos_timer.h"
//#include "user_value.h"

#include <vector>

//When you remove the acceptor classes
/*
typedef struct proposal_status{
    size_t accept_count;
    size_t retain_count;
    std::map<std::string, std::shared_ptr<PAXOS_ID>> acceptors;
    std::shared_ptr<Value>  v;
}PROPOSAL_STATUS;
*/

class paxos_instance{
    std::shared_ptr<Node>	_network_uid;
    std::shared_ptr<PAXOS_ID>	_promised_id;
    std::shared_ptr<PAXOS_ID>	_accepted_id;
    std::shared_ptr<Value>       _accepted_value;
    
    void(*_completed_value_callback)(void*, size_t);
    
    // the learner
    std::map<std::string, PROPOSAL_STATUS*> _proposals;
    std::map<std::string, std::shared_ptr<PAXOS_ID>>        _acceptors;
    std::shared_ptr<Value>                                   _final_value;
    std::map<std::string, std::shared_ptr<PAXOS_ID>>        _final_acceptors;
    std::shared_ptr<PAXOS_ID>                               _final_proposal_id;
    
    // the proposer
    std::shared_ptr<Value>           _proposed_value;
    bool            _leader;
    std::shared_ptr<PAXOS_ID>       _proposal_id;
    std::shared_ptr<PAXOS_ID>       _highest_proposal_id;
    std::shared_ptr<PAXOS_ID>       _highest_accepted_id;
    std::set<std::shared_ptr<Node>> _promises_received;
    std::set<std::string> _nacks_received;
    std::shared_ptr<PAXOS_MSG>      _current_prepare_message;
    std::shared_ptr<PAXOS_MSG>      _current_accept_message;
    
    //proposer*    _proposer;
    //acceptor*    _acceptor;
    //learner*     _learner;
    
    std::string	_state_file;
    
    std::vector<std::shared_ptr<Node>>		_peers;
    size_t                      _quorum_size;
    size_t                      _instance_number;
    std::shared_ptr<Value>                       _current_value;
    
    // These are for the master timer lease stuff
    std::shared_ptr<Node>               _master_uid;
    bool                _master_attempt;
    float               _lease_window;
    float               _lease_start;
    float               _lease_expiry;
    bool                _initial_load;
    TimerThread         _timer;
    TimerThread::timer_id         _lease_timer_id;    
    
    // for the exp_backoff_resolution
    size_t      _backoff_inital;
    size_t      _backoff_cap;
    size_t      _drive_silence_timeout;
    size_t      _retransmit_interval;
    size_t      _backoff_window;
    TimerThread::timer_id      _retransmit_timer;
    //TimerThread                _exp_timer;
    TimerThread::timer_id     _delayed_drive;
    
    
    // for the sync code
    TimerThread _sync_timer;
    float _sync_delay;
    
public:
    paxos_instance(std::shared_ptr<Node> _network_uid, std::vector<std::shared_ptr<Node>> peers, std::string filename, void(*callback)(void*, size_t));
    ~paxos_instance();
    
    void start();
    
    void propose_value(std::shared_ptr<Value> v);

    void receive_prepare(PAXOS_MSG msg);
    void receive_accept(PAXOS_MSG msg);
    void receive_promise(PAXOS_MSG msg);
    void receive_nack(PAXOS_MSG msg);
    void receive_accepted(PAXOS_MSG msg);
    
    void send_accept(std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> proposal_value);
    void send_accept_REAL(std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> proposal_value);
    void send_promise(std::shared_ptr<Node> dest, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> last_accepted_id, std::shared_ptr<Value> last_accepted_value);
    void send_nack(std::shared_ptr<Node> from, size_t instance_num, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<PAXOS_ID> promised_id);
    void send_accepted(size_t instance_number, std::shared_ptr<PAXOS_ID> proposal_id, std::shared_ptr<Value> value);
    void send_prepare(std::shared_ptr<PAXOS_ID> id);
    void send_sync(std::shared_ptr<Node> dest, size_t instance_number);
    void send_catchup(std::shared_ptr<Node> dest, size_t instance_num, std::shared_ptr<Value> value);

    void daemon(Node addr);         // This is for the paxos servers 
    void client_daemon(Node n);    // This is for external client updates
    
    // the supplied function will be called once a value has been agreed_upon
    void set_update_value_callback(void(*callback)(void*, size_t));
private:
    
    void save_state(size_t instance_number, std::shared_ptr<Value> current_value, std::shared_ptr<PAXOS_ID> promised_id, std::shared_ptr<PAXOS_ID> accetped_id, std::shared_ptr<Value> accepted_value);
    void load_state();
    
    
    int handle_propose(PAXOS_MSG msg);
    void handle_prepare(PAXOS_MSG msg);
    void handle_promise(PAXOS_MSG msg);
    void handle_accpt_req(PAXOS_MSG msg);
    void handle_accpted(PAXOS_MSG msg);
    void handle_nack(PAXOS_MSG msg);
    void handle_catchup(PAXOS_MSG msg);
    void handle_sync(PAXOS_MSG msg);
    
    // for the master leasing code
    void start_master_lease_timer();
    void update_lease(std::shared_ptr<Node> master_uid);
    void lease_expired();
    void advance_instance(size_t new_instance_num, std::shared_ptr<Value> new_current_value, bool catchup);
    int propose_update(std::shared_ptr<Value> msg, bool application_level);
    
    // for the drive to resolution
    void reschedule_next_drive_attempt(size_t delay);
    void drive_to_resolution();
    void stop_driving();
    
    //for the sync code
    void sync();
    void receive_sync_request(std::shared_ptr<Node> from, size_t instance_number);
    void receive_catchup(std::shared_ptr<Node> from, size_t instance_number, std::shared_ptr<Value> v);
    
    // the acceptor 
    std::shared_ptr<PAXOS_MSG> acceptor_receive_accept(PAXOS_MSG msg);
    std::shared_ptr<PAXOS_MSG> acceptor_receive_prepare(PAXOS_MSG msg);
    
    // the learner
    std::shared_ptr<PAXOS_MSG> learner_receive_accepted(PAXOS_MSG msg);
    
    // the proposer
    void        proposer_propose_value(std::shared_ptr<Value> v);
    std::shared_ptr<PAXOS_MSG>  proposer_prepare();
    void        proposer_observe_proposal(std::shared_ptr<PAXOS_ID> proposal_id);
    void        proposer_receive_nack(PAXOS_MSG msg);
    std::shared_ptr<PAXOS_MSG>  proposer_receive_promise(PAXOS_MSG msg);
};

#endif /*__PAXOS_INSTANCE__*/