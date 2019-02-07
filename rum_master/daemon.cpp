#include "daemon.h"
//------------------------------------------------------------------------------
typedef enum pkt_type = {
	pkt_query = 0,
	pkt_alloc, 
	pkt_free, 
	pkt_write,
	pkt_read,
} PKT_TYPE;
//------------------------------------------------------------------------------
#define DEBUG 1

#if DEBUG
#define PRINTF(...) {printf("\033[1;34mDEBUG:\033[0m "); printf(__VA_ARGS__);}
#define ERROR(...)	{printf("\033[0;31mERROR:\033[0m "); printf(__VA_ARGS__);}
#define TODO(...)	{printf("\033[0;35mTODO:\033[0m "); printf(__VA_ARGS__);}
#else
#define PRINTF(...)
#define ERROR(...)
#define TODO(...)
#endif
//------------------------------------------------------------------------------
#if DEBUG
const char* debug_pkt_type(PKT_TYPE t){
	switch(){
		case pkt_query:
		return "pkt_query";
		case pkt_alloc:
		return "pkt_alloc";
		case pkt_free:
		return "pkt_free";
		case pkt_read:
		return "pkt_read";
		case pkt_write:
		return "pkt_write";
		default:
		return "UNKONWN";
	}
}
//-------------------
#else
const char* debug_pkt_type(PKT_TYPE t){;}
#endif
//------------------------------------------------------------------------------
static char keep_running = 1;
//------------------------------------------------------------------------------
static void signal_handler(int signum){
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n", signum);
		keep_running = 0;
	}
}
//------------------------------------------------------------------------------
void daemon_init(){
	// catch any kill signals
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);
}
//------------------------------------------------------------------------------
void handle_query(){

}
//------------------------------------------------------------------------------
void handle_alloc(){
	
}
//------------------------------------------------------------------------------
void handle_free(){
	
}
//------------------------------------------------------------------------------
void handle_read(){
	
}
//------------------------------------------------------------------------------
void handle_write(){
	
}
//------------------------------------------------------------------------------
void daemon_listen(){
	
	while(keep_running){
		// get some data

		PRINTF("daemon : receive pkt type %s\n", debug_pkt_type());

		switch(){
			case pkt_query:
				handle_query();
				break;
			case pkt_alloc:
				handle_alloc();
				break;
			case pkt_free:
				handel_free();
				break;
			case pkt_read:
				handle_read();
				break;
			case pkt_write:
				handle_write();
				break;
			default:
		}
	}
}