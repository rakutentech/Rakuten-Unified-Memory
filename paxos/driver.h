#ifndef __DRIVER_H__
#define __DRIVER_H__

#include "util.h"

#include <list>
#include <string>
#include "paxos_instance.h"
#include "paxos_id.h"
#include "comms.h"
#include "paxos_timer.h"
//class comms;

class driver{
	comms 				_messenger;
	NODE				_network_uid;
	std::list<NODE>		_peers;
	size_t				_quorum_size;
	std::string			_state_file;
	paxos_instance		_paxos;

	size_t 				_instance_number;
	VALUE 				_current_value;
	PAXOS_ID			_promised_id;
	PAXOS_ID			_accepted_id;
	VALUE 				_accepted_value;
        
        // for the master node leases
        NODE                _master_uid;
        bool                _master;
        float               _lease_window;
        float               _lease_start;
        float               _lease_expiry;
        bool                _master_attempt;
        bool                _initial_load;
        Paxos_timer         _lease_timer;
        Paxos_timer::timer_id             _timer_id;
        
        // for the exp_backoff_resolution
        size_t      _backoff_inital;
        size_t      _backoff_cap;
        size_t      _drive_silence_timeout;
        size_t      _retransmit_interval;
        size_t      _backoff_window;
        Paxos_timer::timer_id      _retransmit_task;
        Paxos_timer _exp_timer;
        Paxos_timer::timer_id     _delayed_drive;

public:
	driver(NODE n, std::list<NODE> peer_list, std::string state_f);
	void set_messenger(comms c);
private:
	void load_state();
	void save_state(size_t inst_number, VALUE curr_val, PAXOS_ID prom_id, PAXOS_ID acpt_id, VALUE acpt_val);
	
	void propose_update(VALUE v);
	void advance_instance(size_t new_instance_number, VALUE new_current_value);
	void send_prepare(PAXOS_ID proposal_id);
	void send_accept(PAXOS_ID proposal_id, VALUE proposal_value);
	void send_accepted(PAXOS_ID proposal_id, VALUE proposal_value);
	void receive_prepare(NODE from, size_t inst_number, PAXOS_ID proposal_id);
	void receive_nack(NODE from, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID promised_proposal_id);
	void receive_promise(NODE from, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID last_accepted_id, VALUE last_accepted_value);
	void receive_accept(NODE from, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value);
	void receive_accepted(NODE from, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value);
        
        // master lease node
        void start_master_lease_timer();
        void update_lease(NODE master_uid);
        void lease_expired();
        void propose_update(VALUE new_val, bool application_lvl);
        
        // exp_backoff_resolution
        void reschedule_next_drive_attempt(size_t delay);
        void drive_to_resolution();
        void stop_driving();
};
#endif /*__DRIVER_H__*/