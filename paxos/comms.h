#ifndef __COMMS_H__
#define __COMMS_H__

#include "util.h"

#include <map>
#include <list>
#include "paxos_id.h"
//#include "driver.h"
#include "node.h"

class comms{
	std::map<Node*, IP_ADDR> _addrs;
	std::map<IP_ADDR, Node*> _nodes;
	//driver					_driver;

public:
    comms();
    comms(Node* me, std::list<Node*> peer_addrs);//, driver d);
    ~comms();
    
    // the management functions
    void start_protocol();
    void udp_received();
    
    // the interaction functions
    void send_sync_request(Node peer_uid, size_t instance_number);
    void send_catchup(Node peer_uid, size_t instance_number, VALUE current_value);
    void send_nack(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID promised_proposal_id);
    void send_prepare(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id);
    void send_promise(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID last_accepted_id, VALUE last_accepted_value);
    void send_accept(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value);
    void send_accepted(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value);
        
private:
    int                 f_socket;
    int                 f_port;
    std::string         f_addr;
    struct addrinfo *   f_addrinfo;
    
    void daemon(const std::string& addr, int port);
};

#endif /*__COMMS_H__*/