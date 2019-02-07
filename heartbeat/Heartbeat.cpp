#include "Heartbeat.h"
#include "paxos_timer.h"		// for the callbacks on timeout
#include <mutex>
#include <thread>
#include <chrono>
#include <string.h>
#include "util.h"
//------------------------------------------------------------------------------
// should specify the units
#define TIMEOUT_LEN_MS 			(std::chrono::milliseconds(10000))
#define TRANS_WAIT_MS			(std::chrono::milliseconds(5000))

#define TIMEOUT_LEN_MS_ABS 		(10000)
#define TRANS_WAIT_MS_ABS		(5000)

#define HEARTBEAT_RECV_PORT 	45557
#define HEARTBEAT_SEND_PORT 	45557
//------------------------------------------------------------------------------
Heartbeat::Heartbeat(std::string slave_ip, void (*cbf)(std::string)){
	// this constructor is for the slave
	transmit 		= false;
	this_node 		= slave_ip;
	user_callback 	= cbf;
}
//------------------------------------------------------------------------------
Heartbeat::Heartbeat(unsigned char* uuid){
	// this constructor is for a client
	transmit = true;
	memcpy(digest, uuid, MD5_STR_LENGTH);
}
//------------------------------------------------------------------------------
Heartbeat::~Heartbeat(){

	transmission_running 	= false;
	reception_running  		= false;

	if(transmit){
		send_thread.join();
		slave_map.clear();
	}
	else{
		recv_thread.join();
		client_map.clear();
	}

	//delete timer_thread;
}
//------------------------------------------------------------------------------
void Heartbeat::start_daemon(){
	if(transmit){
		// need to start a thread here to do the heartbeats	
		send_thread = std::thread(&Heartbeat::transmit_thread, this);
		transmission_running = true;
		PRINTF("start_daemon - started transmit\n");
	}
	else{
		// need to start a thread to be the watchdog
		recv_thread = std::thread(&Heartbeat::reception_thread, this);
		reception_running = true;
		PRINTF("start_daemon - started reception\n");
	}
}
//------------------------------------------------------------------------------
void Heartbeat::stop_daemon(){
	PRINTF("Heartbeat::stop_daemon\n");
	if(transmit && transmission_running){
		transmission_running = false;
		send_thread.join();
		PRINTF("stop_daemon - stopped transmit\n");
	}
	else if(reception_running){
		reception_running = false;
		PRINTF("stop_daemon - stopped reception\n");
	}
}
//------------------------------------------------------------------------------
// Client Functions
//------------------------------------------------------------------------------
void Heartbeat::add_slave(std::string their_ip){
	PRINTF("Heartbeat::add_slave : %s\n", their_ip.c_str());
	
	UDP_comms *tmp = new UDP_comms(their_ip, HEARTBEAT_SEND_PORT, false);

	// this tells the transmit thread to do heartbeats
	std::lock_guard<std::mutex> lock(hb_mutex);
	slave_map.insert({their_ip, tmp});

	// this registers the client with the slave
	HB_MSG msg;
	msg.type = MSG_REGISTER;
	memcpy(msg.hash, digest, MD5_STR_LENGTH);
	tmp->send((char*)&msg, sizeof(HB_MSG));
}
//------------------------------------------------------------------------------
void Heartbeat::remove_slave(std::string their_ip){
	PRINTF("Heartbeat::remove_slave : %s\n", their_ip.c_str());
	std::map<std::string, UDP_comms*>::iterator slave_iter;
	
	std::lock_guard<std::mutex> lock(hb_mutex);

	if((slave_iter = slave_map.find(their_ip)) != slave_map.end())
	{
		delete slave_iter->second;
		slave_map.erase(slave_iter);
	}
}
//------------------------------------------------------------------------------
/**
 * For each known slave, send them a keep alive mesage every TRANSMISSION_WAIT_S
 */
void Heartbeat::transmit_thread(){
	std::map<std::string, UDP_comms*>::iterator slave_iter;
	HB_MSG msg;

	msg.type = MSG_HEARTBEAT;
	memcpy(msg.hash, digest, MD5_STR_LENGTH);

	while(transmission_running)
	{
		{
			std::lock_guard<std::mutex> lock(hb_mutex);
			for(slave_iter = slave_map.begin() ; slave_iter != slave_map.end() ; slave_iter++)
			{
				//PRINTF("transmit_thread: send to %s\n", ((std::string)slave_iter->first).c_str());
				((UDP_comms*)slave_iter->second)->send((char*)&msg, sizeof(msg));
			}
			
		}
		std::this_thread::sleep_for(TRANS_WAIT_MS);
	}
}
//------------------------------------------------------------------------------
// Slave Functions
//------------------------------------------------------------------------------
/**
 * When this function is called, we assume that the client is dead, so act....
 */
void Heartbeat::callback(std::string uuid){
	PRINTF("Heartbeat::callback\n");
	
	// some kind of call up
	user_callback(uuid);

	// remove the client from the list
}
//------------------------------------------------------------------------------
void Heartbeat::create_timeout(HB_MSG msg){
	//PRINTF("Heartbeat::create_timeout\n");
	std::string hash(msg.hash);
	hash.assign(msg.hash, MD5_STR_LENGTH);
	if(client_map.count(hash) > 0){
		// already exists
		PRINTF("Heartbeat::create_timeout - already exists -> %s\n", msg.hash);
		return;
	}

	//insert something
	TimerThread::timer_id tmp_id = timer_thread.addTimer(TIMEOUT_LEN_MS_ABS, 0, [=](){callback(hash);});
	client_map.insert({hash, tmp_id});

	PRINTF("Heartbeat::create_timeout - %s (%ld bytes), %ld\n", hash.c_str(), hash.size(), tmp_id);
}
//------------------------------------------------------------------------------
int Heartbeat::reset_timeout(HB_MSG msg){
	PRINTF("Heartbeat::reset_timeout\n");
	std::lock_guard<std::mutex> lock(hb_mutex);

	std::map<std::string, TimerThread::timer_id>::iterator itter;

	// find the timer
	std::string hash;
	hash.assign(msg.hash, MD5_STR_LENGTH);
	if((itter = client_map.find(hash)) == client_map.end()){
		// could not find the timer

		TODO("Heartbeat::reset_timeout - could not find '%s', " \
			"so it timedout. Should try and delete relevant memory\n", msg.hash);
		return 0;
	}

	// delete the timer
	timer_thread.clearTimer(((TimerThread::timer_id)itter->second));

	// create and record a new timeout
	TimerThread::timer_id tmp_id = timer_thread.addTimer(TIMEOUT_LEN_MS_ABS, 0, [=](){callback(hash);});
	client_map[hash] = tmp_id;

	return 1;
}
//------------------------------------------------------------------------------
void Heartbeat::reception_thread(){
	PRINTF("reception_thread - %s:%d\n", this_node.c_str(), HEARTBEAT_RECV_PORT);
	// create an endpoint to 
	UDP_comms *conn = new UDP_comms(this_node, HEARTBEAT_RECV_PORT, true);

	HB_MSG msg;

	while(reception_running){
		// get packets
		conn->recv((char*)&msg, sizeof(HB_MSG));

		PRINTF("reception_thread: %s - %s\n", 
			((msg.type == MSG_REGISTER)?"MSG_REGISTER":"MSG_HEARTBEAT"), msg.hash);

		if(msg.type == MSG_HEARTBEAT){
			if(!reset_timeout(msg))
			{
				// could not find a timer to reset
			}	
		}
		else if(msg.type == MSG_REGISTER){
			create_timeout(msg);
		}
	}
	delete conn;
}
//------------------------------------------------------------------------------
void Heartbeat::dump_lists(){
	std::map<std::string, TimerThread::timer_id>::iterator c_iter;
	std::map<std::string, UDP_comms*>::iterator 		s_iter;
	size_t count = 0;

	if(transmit){
		// dump the slave_map
		for(s_iter = slave_map.begin() ; s_iter != slave_map.end() ; s_iter++)
		{
			printf("[%ld] <> <>\n", count++); //, s_iter->first, s_iter->second);
		}
	}
	else{
		// dump the client_map
		for(c_iter = client_map.begin() ; c_iter != client_map.end() ; c_iter++)
		{
			printf("[%ld] %s <>\n", count++, c_iter->first.c_str());//client_iter->second);
		}
	}
}
//------------------------------------------------------------------------------
