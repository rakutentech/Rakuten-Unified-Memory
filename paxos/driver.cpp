#include "driver.h"

#include <fstream>
#include <iostream>
//---------------------------------------------------------
driver::driver(Node n, std::list<Node> peer_list, std::string state_f){
	_messenger 		= comms();
	_network_uid	= n;
	_peers 			= peer_list;
	_quorum_size	= (_peers.size() / 2) + 1;
	_state_file		= state_f;

	// restore state is possible
	load_state();

	// create a paxos instance
        _paxos = paxos_instance(n, _quorum_size, PAXOS_ID(0, ""), PAXOS_ID(0, ""), VALUE_NULL);
	TODO("DRIVER: create paxos\n");

	_current_value 	= VALUE_NULL;
        
        // for the master strategy
        //_timer = Paxos_timer();
        
        // for the exp_backoff_resolution
        // time in millisecs
        _backoff_inital     = 5;
        _backoff_cap        = 2000;
        _drive_silence_timeout   = 3000;
        _retransmit_interval = 1000;
        _backoff_window     = _backoff_inital;
        //_retransmit_task    = 0;
        //_delayed_drive      = 0;
}
//---------------------------------------------------------
void driver::set_messenger(comms c){
	_messenger = c;
}
//---------------------------------------------------------
void driver::save_state(size_t inst_number, VALUE curr_val, PAXOS_ID prom_id, PAXOS_ID acpt_id, VALUE acpt_val){
	std::ofstream file;

	_instance_number	= inst_number;
	_current_value		= curr_val;
	_promised_id		= prom_id;
	_accepted_id		= acpt_id;
	_accepted_value		= acpt_val;

	std::string tmp = _state_file + "tmp";
	
	file.open(tmp);
	if(!file){
		ERROR("LOAD_STATE: problem loading state file.\n");
	}

	file << _instance_number 		<< std::endl;
	file << _current_value 			<< std::endl;
	file << _promised_id.to_str() 	<< std::endl;
	file << _accepted_id.to_str()	<< std::endl;
	file << _accepted_value 		<< std::endl;

	file.flush();
	file.close();

	// rename() is an atomic filesystem operation. By writing the new
    // state to a temporary file and using this method, we avoid the potential
    // for leaving a corrupted state file in the event that a crash/power loss
    // occurs in the middle of update.
	if(rename((_state_file + "tmp").c_str(), _state_file.c_str()) != 0){
		ERROR("SAVE_STATE: there was a problem renaming the file\n");
	}
}
//---------------------------------------------------------
void driver::load_state(){
    
    
    // master strategy
    if(_initial_load){
        _initial_load = false;
        update_lease(NODE_NULL);
    }
    
    // normal paxos
    std::fstream file;

    file.open(_state_file, std::ios::out | std::ios::in);

    if(!file){
            // file does not exit, make it
    file << _instance_number 		<< std::endl;
    file << _current_value 			<< std::endl;
    file << _promised_id.to_str() 	<< std::endl;
    file << _accepted_id.to_str()	<< std::endl;
    file << _accepted_value 		<< std::endl;

    }
    else{
            std::string line;

            std::getline(file, line);
            _instance_number = std::stoi(line);

            std::getline(file, line);
            _current_value = line;	// todo

            std::getline(file, line);
            _promised_id;// = line;	// todo

            std::getline(file, line);
            _accepted_id;// = line;	// todo

            std::getline(file, line);
            _accepted_value = line;	// todo

    }

    file.flush();
    file.close();
}
//---------------------------------------------------------
// this should be overridden
void driver::propose_update(VALUE v){
    
    // master strategy
    
    // resolution strategy
    drive_to_resolution();
    
    if(_paxos.proposed_value == VALUE_NULL){
            _paxos.propose_value(v);
    }
}
//---------------------------------------------------------
// something about catchup?
void driver::advance_instance(size_t new_instance_number, VALUE new_current_value){
 
    // master strategy
    
    // resolution strategy
    stop_driving();
    _backoff_window = _backoff_inital;
    
    save_state(new_instance_number, new_current_value, PAXOS_ID(0, ""), PAXOS_ID(0, ""), VALUE_NULL);

    _paxos = paxos_instance(_network_uid, _quorum_size, PAXOS_ID(0, ""), PAXOS_ID(0, ""), VALUE_NULL);

    PRINTF("UPDATED: %ld %s\n", new_instance_number, new_current_value.c_str());
}
//---------------------------------------------------------
void driver::send_prepare(PAXOS_ID proposal_id){
	for(auto i : _peers){
		_messenger.send_prepare(i, _instance_number, proposal_id);
	}
}
//---------------------------------------------------------
void driver::send_accept(PAXOS_ID proposal_id, VALUE proposal_value){
    for(auto i : _peers){
            _messenger.send_accept(i, _instance_number, proposal_id, proposal_value);
    }
    
    // resolution strategy
    if(_exp_timer.exists(_retransmit_task)){
        _exp_timer.destroy(_retransmit_task);
    }
    //_retransmit_task = _exp_timer.create(0, _retransmit_interval, [=](){send_accept(_proposal_id, _proposal_value});
    
}
//---------------------------------------------------------
void driver::send_accepted(PAXOS_ID proposal_id, VALUE proposal_value){
	for(auto i : _peers){
		_messenger.send_accepted(i, _instance_number, proposal_id, proposal_value);
	}
}
//---------------------------------------------------------
void driver::receive_prepare(Node from, size_t inst_number, PAXOS_ID proposal_id){
    
    // master strategy
    if(_master && from != _master_uid){
        // drop non-master requests
        return;
    }
    
    // only process messages for the current link in the multi paxos chain
    if(_instance_number != inst_number){
            return;
    }

    PAXOS_MSG m = _paxos.receive_prepare(PAXOS_MSG::prepare(from, proposal_id));

    if(m.get_type() == promise_enum){
            save_state(_instance_number, _current_value, m.get_proposal_id(), m.get_last_accepted_id(), m.get_last_accepted_value());

            _messenger.send_promise(from, _instance_number, m.get_proposal_id(), m.get_last_accepted_id(), m.get_last_accepted_value());
    }
    else{
            _messenger.send_nack(from, _instance_number, proposal_id, _promised_id);
    }
}
//---------------------------------------------------------
void driver::receive_nack(Node from, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID promised_proposal_id){
	// only process messages for the current link in the multi paxos chain
	if(_instance_number != instance_number){
		return;
	}

	_paxos.receive_nack(PAXOS_MSG::nack(from, _network_uid, proposal_id, promised_proposal_id));
}
//---------------------------------------------------------
void driver::receive_promise(Node from, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID last_accepted_id, VALUE last_accepted_value){
	// only process messages for the current link in the multi paxos chain
	if(_instance_number != instance_number){
		return;
	}

	PAXOS_MSG m = _paxos.receive_promise(PAXOS_MSG::promise(from, _network_uid, proposal_id, last_accepted_id, last_accepted_value));

	if(m.get_type() == accept_req_enum){
		send_accept(m.get_proposal_id(), m.get_proposal_value());
	}
}
//---------------------------------------------------------
void driver::receive_accept(Node from, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value){
    
    // master strategy
    if(_master && from != _master_uid){
        // drop non-master requests
        return;
    }
    
    // only process messages for the current link in the multi paxos chain
    if(_instance_number != instance_number){
            return;
    }

    PAXOS_MSG m = _paxos.receive_accept(PAXOS_MSG::accept(from, proposal_id, proposal_value));

    if(m.get_type() == accepted_enum){
            save_state(_instance_number, _current_value, _promised_id, proposal_id, proposal_value);

            send_accepted(m.get_proposal_id(), m.get_proposal_value());
    }
    else{
            _messenger.send_nack(from, _instance_number, proposal_id, _promised_id);
    }
}
//---------------------------------------------------------
void driver::receive_accepted(Node from, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value){
	// only process messages for the current link in the multi paxos chain
	if(_instance_number != instance_number){
		return;
	}

	PAXOS_MSG m = _paxos.receive_accepted(PAXOS_MSG::accepted(from, proposal_id, proposal_value));

	if(m.get_type() == resolution_enum){
		advance_instance(_instance_number + 1, proposal_value);
	}
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
/**
 * The following code deals with the lease nature of leader elections
 */
//---------------------------------------------------------
void driver::start_master_lease_timer(){
    //_lease_start = now;
    
    //if(_lease_expiry && not explired){
    if(_lease_timer.exists(_timer_id)){
        // cancel the timer
        _lease_timer.destroy(_timer_id);
    }

    // starting from now
    _timer_id  = _lease_timer.create(0, _lease_window, std::bind(&driver::lease_expired, this));
}
//---------------------------------------------------------
void  driver::update_lease(Node master_uid){
    _master_uid = master_uid;
    
    if(_network_uid != _master_uid){
        start_master_lease_timer();
    }
    
    if(_network_uid == _master_uid){
        //auto renew_delay  = (_lease_start + _lease_window - 1) - now();
        
        //if(renew_delay > 0){
        if(_lease_timer.exists(_timer_id)){
            //_timer_id  = _timer.create(0, _lease_window, propose_update(_network_uid, false));
            _timer_id  = _lease_timer.create(0, _lease_window, [=](){propose_update(_network_uid, false);});
        }
        else{
            propose_update(_network_uid, false);
        }
    }
}
//---------------------------------------------------------
void  driver::lease_expired(){
    _master = false;
    propose_update(_network_uid, false);
}
//---------------------------------------------------------
void  driver::propose_update(VALUE new_val, bool application_lvl){
    if(application_lvl){
        if(_master_uid == _network_uid){
            propose_update("SOME VALUE");
        }
        else{
            PRINTF("IGNORE CLIENT REQUEST, master is %s\n", _master_uid.c_str());
        }
    }
    else{
        if((_master_uid == NODE_NULL || _master_uid == _network_uid) && !_master_attempt){
            _master_attempt = true;
            start_master_lease_timer();
            propose_update("SOME VALUE");
        }
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
void driver::reschedule_next_drive_attempt(size_t delay){
    if(_delayed_drive != 0 && _exp_timer.exists(_delayed_drive)){
        _exp_timer.destroy(_delayed_drive);
    }
    
    _delayed_drive = _exp_timer.create(0, delay, [=](){drive_to_resolution();});
}
//---------------------------------------------------------
void driver::drive_to_resolution(){
    stop_driving();
    
    // advance to the next proposal number
    TODO("_paxos_prepre\n");
    PAXOS_MSG m;// = _paxos.prepare();
    
    _retransmit_task = _exp_timer.create(0, _retransmit_interval/1000, [=,&m](){send_prepare(m.get_proposal_id());});
}
//---------------------------------------------------------
void driver::stop_driving(){
    if(_exp_timer.exists(_retransmit_task)){
        _exp_timer.destroy(_retransmit_task);
        //_retransmit_task = -1;
    }
    
    if(_exp_timer.exists(_delayed_drive)){
        _exp_timer.destroy(_delayed_drive);
    }
}
//---------------------------------------------------------
//---------------------------------------------------------
//---------------------------------------------------------
        