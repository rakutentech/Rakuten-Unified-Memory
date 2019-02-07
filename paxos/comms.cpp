#include "comms.h"
//---------------------------------------------------------
comms::comms(){

}
//---------------------------------------------------------
comms::~comms(){
    
}
//---------------------------------------------------------
comms::comms(Node* me, std::list<Node*> peer_addrs){//, driver d){
    TODO("comms: create/copy the list of peers\n");
        //_addrs 		= peer_addrs;
	//_driver		= d;

	// two map between names and addrs
	for(auto ad : _addrs){
		_nodes[ad.second] = ad.first;
	}
}
//---------------------------------------------------------
void comms::start_protocol(){
	//_driver.set_messenger(/*this*/);
    TODO("set the driver up\n");
}
//---------------------------------------------------------
void comms::udp_received(){

}
//---------------------------------------------------------
// this is the basic send function
void _send(){
	;
}
//---------------------------------------------------------
void comms::send_sync_request(Node peer_uid, size_t instance_number){
	TODO("send_catchup\n");
}
//---------------------------------------------------------
void comms::send_catchup(Node peer_uid, size_t instance_number, VALUE current_value){
	TODO("send_catchup\n");
}
//---------------------------------------------------------
void comms::send_nack(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID promised_proposal_id){
	TODO("send_nack\n");
}
//---------------------------------------------------------
void comms::send_prepare(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id){
	TODO("send_prepare\n");
}
//---------------------------------------------------------
void comms::send_promise(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, PAXOS_ID last_accepted_id, VALUE last_accepted_value){
	TODO("send_promise\n");
}
//---------------------------------------------------------
void comms::send_accept(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value){
	TODO("send_accept\n");
}
//---------------------------------------------------------
void comms::send_accepted(Node peer_uid, size_t instance_number, PAXOS_ID proposal_id, VALUE proposal_value){
	TODO("send_accepted\n");
}
//---------------------------------------------------------
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdexcept>
#include <string.h>
#include <unistd.h>
void comms::daemon(const std::string& addr, int port){
    
    f_port  = port;
    f_addr  = addr;
    
    char decimal_port[16];
    snprintf(decimal_port, sizeof(decimal_port), "%d", f_port);
    decimal_port[sizeof(decimal_port) / sizeof(decimal_port[0]) - 1] = '\0';
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
    int r(getaddrinfo(addr.c_str(), decimal_port, &hints, &f_addrinfo));
    if(r != 0 || f_addrinfo == NULL)
    {
        ERROR("invalid address\n");
        //throw udp_client_server_runtime_error(("invalid address or port: \"" + addr + ":" + decimal_port + "\"").c_str());
    }
    f_socket = socket(f_addrinfo->ai_family, SOCK_DGRAM | SOCK_CLOEXEC, IPPROTO_UDP);
    if(f_socket == -1)
    {
        freeaddrinfo(f_addrinfo);
        ERROR("Could not create socket\n");
        //throw udp_client_server_runtime_error(("could not create socket for: \"" + addr + ":" + decimal_port + "\"").c_str());
    }
}
//---------------------------------------------------------
//---------------------------------------------------------