#ifndef __HEARBEAT_H__
#define __HEARBEAT_H__

#include <string>

#include <UDP_comms.h>
#include <paxos_timer.h>

#include <openssl/md5.h>
#include <map>

typedef int MAC_ADDR;

#define MSG_REGISTER 	0
#define MSG_HEARTBEAT	1

#define MD5_STR_LENGTH (MD5_DIGEST_LENGTH * 2 + 1)

typedef struct hb_msg{
	char type;
	char hash[MD5_STR_LENGTH];
} HB_MSG;


class Heartbeat
{
public:

	/**
	* This is for creating the client
	*/
	Heartbeat(unsigned char* uuid);

	/**
	* This is for creating the server
	*/
	Heartbeat(std::string slave_ip, void (*cbf)(std::string));
	~Heartbeat();

	void start_daemon();
	void stop_daemon();

	// client functions
	void add_slave(std::string their_ip);
	void remove_slave(std::string their_ip);

	//debug
	void dump_lists();

private:

	unsigned char digest[MD5_STR_LENGTH]; // hash of user-provided uuid
	std::string this_node;
	std::thread recv_thread;
	std::thread send_thread;

	bool transmit;
	bool reception_running = false;
	bool transmission_running = false;

	std::map<std::string, UDP_comms*> 			slave_map; 		// for the transmitters
	std::map<std::string, TimerThread::timer_id> client_map;	// for the receiver	

	TimerThread timer_thread;

	std::mutex hb_mutex;

	// client
	void transmit_thread();

	void (*user_callback)(std::string);

	// server
	void callback(std::string uuid);
	void create_timeout(HB_MSG msg);
	int  reset_timeout(HB_MSG msg);
	void reception_thread();
};

#endif /*__HEARBEAT_H__*/