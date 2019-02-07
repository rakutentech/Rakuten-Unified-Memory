// for the ucontext
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
//#include <stdlib.h>         // for rand
#include <cstdlib>			// at_exit

#include <errno.h>
#include <string.h>
//#include <string>

#include <signal.h>
#include <unistd.h>

#include <sys/mman.h>	// memory protection
#include <ucontext.h>	// for the reg access
#include <limits.h>		
#include <stdint.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include <math.h>

// for the open()
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <map>			// for the local routing
#include <list>
#include <vector>
#include <unordered_set>

#include <algorithm>

#include <mutex>          // std::mutex
#include <thread>         // std::thread

#include "rum.h"
#include "rum_c_version.h"
#include "dpdk_transition.h"		// this gives Addr
#include "util.h"
#include "address_lru.hpp"

//#include "fsm.h"				// for local fsm or remote memory

#include "Heartbeat.h"

#include <boost/filesystem.hpp>

//#include "./readerwriterqueue/readerwriterqueue.h"


using namespace std;
//using namespace moodycamel;
//------------------------------------------------------------------------------
/**
 * Thinking space
 * - should i create a new pointer type with RUM accounting info
 * 		- This would include the remote memory address locations
 * - Should this all be decentralised or (for speed) should we be having manager nodes
 *		- can manager nodes also be normal nodes?
 * - At what point should i start going off node
 * 		- no more physical RAM left?
 *		- some sort of frequently accessed memory metric
 * - How to do performant eventual consistency?
 * - Configurable replication ? 
 * - Snapshots....?
 */
//------------------------------------------------------------------------------
/**
TODO : all the maps should be merged into map<Addr, struct act>, 
	struct act{
		Addr remote_ptr;
		Addr start;
		Addr end;
		size_t size;
		HASH_TYPE h;
		vector<Location>	slaves_alts;
		char dirty_bit;
		....
	}
*/
typedef struct accouting{
	//char 				dirty_bit;			// has this been written to cache
	Addr 				start;				// local start ptr
	Addr 				end;				// local end of malloc'd space
	Addr 				remote;				// remote ptr
	size_t 				size;				// size requested to be allocated
	size_t 				alloc_size; 		// size actually allocated
	char 				has_hash;			// Memory is associated with a hash?
	HASH_TYPE			h;					// hash of this memory location
	vector<Location> 	*replicas;			// location(s) of the remote memory
} ACT_STRUCT;

static map<Addr, ACT_STRUCT> 	data_map;	// map of the allocated memory
// THIS SHOULD BE A BIT MAP....
static map<Addr, char> 			dirty_map; 	// map of the dirty bits
//------------------------------------------------------------------------------
#define DEBUG 1
//------------------------------------------------------------------------------
// some simple conversion functions for sizes
#define BYTES_TO_KB(a) 		((a) * 1024)
#define BYTES_TO_MB(a) 		(BYTES_TO_KB(a) * 1024)
#define BYTES_TO_GB(a) 		(BYTES_TO_MB(a) * 1024)

#define ADDR(a)				((Addr)(a))
#define DATA(a, offset)  	(*(((char*)(a)) + offset))

#define TO_STR(a)			(&((a).hash_str[0]))
//------------------------------------------------------------------------------
#define RAW 0
//------------------------------------------------------------------------------


// this stores the location of where the pointers are stored
//typedef map<Addr, vector<Location>*> ADDRESS_MAP;
//------------------------------------------------------------------------------
//static ADDRESS_MAP 			addr_map; 		// map of pointers to slaves
//static map<Addr, size_t>	pointer_size_map;	// store pointer's memory sizes
// we need this as memory can be free'd
static map<Addr, Addr>		addr_space_map;		// Map of start and end address

//static map<Addr, Addr>		slave_ptr_map;			// map of local_ptr to remote ptr
//static map<Addr, HASH_TYPE>	slave_ptr_hash_map;		// map of local to slave ptr (hash)

static int RMEM_PG_SIZE = 0;
static size_t RMEM_PAGE_BASE_MASK = 0;
static size_t memblk_cache_size = 1000;

static AddressLRU local_mem_block_cache;
static std::unordered_set<Addr> dirty_pages;

static vector<Location>		slaves;

static char initialised = 0;

static mutex				memory_mutex;

static Heartbeat *client;			// for the keep alive messages
static unsigned char uuid[MD5_STR_LENGTH];		// uuid for this session
//------------------------------------------------------------------------------
// accouting 
static size_t rum_alloc_bytes 	= 0;
static size_t rum_free_bytes 	= 0;
static size_t mmap_alloc_bytes  = 0;
static size_t mmap_free_bytes   = 0;
//------------------------------------------------------------------------------
// accounting for the memory accesses
typedef struct mem_access{
    char write;
    Addr addr;
    Addr base_ptr;
}MEM_ACCESS;

//#define MEM_ACCESS_BUFFER_SIZE	10240
//static moodycamel::ReaderWriterQueue<MEM_ACCESS> memory_q(MEM_ACCESS_BUFFER_SIZE);
//------------------------------------------------------------------------------
void signal_callback_handler(int signum, siginfo_t *info, void *extra);
//------------------------------------------------------------------------------
//void signal_callback_handler2(int signum, siginfo_t *info, void *extra);
//void process_pages(void);
// thread to process the memory operations
static thread processing_thread;
//------------------------------------------------------------------------------
void dump_act(ACT_STRUCT a){
	//printf("\tdirty_bit	: %s\n",  a.dirty_bit?"TRUE":"FALSE");
	printf("\tstart		: %lx\n", a.start);
	printf("\tend		: %lx\n", a.end);
	printf("\tremote		: %lx\n", a.remote);
	printf("\tsize		: %ld\n", a.size);
	printf("\talloc_size	: %ld\n", a.alloc_size);
	if(a.has_hash){
		printf("\thash		: %s\n",  a.h.hash_str);
	}
	else{
		printf("\thash		: %s\n", "<NO HASH>");	
	}
	for(vector<Location>::iterator it=a.replicas->begin();it != a.replicas->end(); it++){
		printf("\t");location_print(*it);
	}
}
//------------------------------------------------------------------------------
void debug_dump_lists(){
	int count = 0;
	printf("Data map\n");
	for(map<Addr, ACT_STRUCT>::iterator a=data_map.begin(); a!=data_map.end(); a++){
		printf("[%d]\n", count++);
		dump_act(a->second);
	}
}
//------------------------------------------------------------------------------
void dump_dirty(){
	int count = 0;
	printf("Dirty map\n");
	for(map<Addr, char>::iterator a=dirty_map.begin(); a!=dirty_map.end(); a++){
		printf("[%d] %lx %d\n", count++, a->first, a->second);
	}
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// this is based on the openssl library
// http://www.askyb.com/cpp/openssl-md5-hashing-example-in-cpp/
// will require the -lcrypto
void hash_func(const char* str, unsigned char* digest_str){
	// do some hashing

	static const char hexDigits[17] = "0123456789ABCDEF";
	unsigned char digest[MD5_DIGEST_LENGTH];
	//PRINTF("HASH: %s (len %ld)\n", str, strlen(str));

	MD5( (const unsigned char*) str, strlen(str), digest);

	// assume that the digest_str is 33 bytes long
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++){
		digest_str[i*2] 		= hexDigits[(digest[i] >> 4) & 0xF];
		digest_str[(i*2) + 1] 	= hexDigits[digest[i] & 0xF];
	}
	digest_str[MD5_DIGEST_LENGTH * 2] = '\0';

#if 0
	char mdString[33];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
 
    //PRINTF("md5 digest: %s\n", mdString);
    PRINTF("md5 digest_str: %s\n", digest_str);
#endif
}
//------------------------------------------------------------------------------
// take the mac address of the first device on the system
void get_mac_address(uint8_t *mac){
	std::string path 		= "/sys/class/net";
	std::string addr_path	= "/address";
	std::string dev_path;

    for (auto & p : boost::filesystem::directory_iterator(path))
    {
        //printf("%s\n", p.path().c_str());
        //printf("%s\n", p.path().leaf().c_str());
        dev_path = p.path().string();
        break;
    }

    // now we have the device, get the address
    char buf[256];
	FILE *fp = fopen((dev_path + addr_path).c_str(), "rt");
	memset(buf, 0, 256);
	if (fp) {
		if (fgets(buf, sizeof buf, fp) != NULL) {
			sscanf(buf, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0],
								&mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
		}
		fclose(fp);
	}
	//printf("ADDR: %s\n", buf);
	//printf("My Mac Address - %hhx:%hhx:%hhx:%hhx:%hhx:%hhx\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}
//------------------------------------------------------------------------------
//TODO: yes Kunal, i know that the above should be somewhere else...
void generate_uuid(unsigned char* uuid){
	uint8_t mac[6];
	char mac_addr[18];
	
	get_mac_address(mac);
	sprintf(mac_addr, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	hash_func(mac_addr, uuid);
}
//------------------------------------------------------------------------------
std::vector<std::string> getNextLineAndSplitIntoTokens(std::istream& str)
{
    std::vector<std::string>   result;
    std::string                line;
    std::getline(str,line);

    std::stringstream          lineStream(line);
    std::string                cell;

    while(std::getline(lineStream,cell, ','))
    {
        result.push_back(cell);
    }
    // This checks for a trailing comma with no data after it.
    if (!lineStream && cell.empty())
    {
        // If there was a trailing comma then add an empty element.
        result.push_back("");
    }
    return result;
}
//------------------------------------------------------------------------------
// inspired by https://github.com/rfjakob/earlyoom/blob/master/meminfo.c
// returns avaliable memory available in bytes
size_t get_avail_memory(){
	//cat /proc/meminfo
	FILE* 	fd;
	char 	buf[8192];

	size_t 	len;
	char* 	hit_str;
	size_t 	val;

	// first open the file
	if((fd = fopen("/proc/meminfo", "r")) == NULL){
		fprintf(stderr, "could not open /proc/meminfo: %s\n", strerror(errno));
		return 0;
	}
	rewind(fd);

	// then read the file
	len = fread(buf, 1, sizeof(buf) - 1, fd);
    if (len == 0) {
        fprintf(stderr, "could not read /proc/meminfo: %s\n", strerror(errno));
        return 0;
    }

    // zero terminated string
    buf[len] = 0;

    hit_str = strstr(buf, "MemFree:");
    if(hit_str == NULL){
    	fprintf(stderr, "unable to find 'MemFree' in /proc/meminfo\n");
    	return 0;
    }

    errno = 0;
    val = strtol(hit_str + strlen("MemFree:"), NULL, 10);
    if(errno != 0){
    	perror("get_entry: strtol() failed");
        return -1;
    }

    PRINTF("rum_: avaliable mem %ld MB\n", (val/1024));
	//return val * 1024;
    return 5;
}
//------------------------------------------------------------------------------
void rum_init(const char* machines_csv_file_path){
	if(initialised){
		return;
	}

	std::lock_guard<std::mutex> lock(memory_mutex);

	// register this function to be called at the end
	std::atexit(rum_end);

	initialised = 1;

	// record the pagesize
	RMEM_PG_SIZE = sysconf(_SC_PAGESIZE);
	PRINTF("RMEM_INIT: page size %d\n", RMEM_PG_SIZE);

	// register the signal handler to catch the seg faults
	struct sigaction segf_action;
	memset(&segf_action, 0, sizeof(struct sigaction));

	// for the segmentation faults
	segf_action.sa_flags = SA_SIGINFO;
	sigemptyset(&segf_action.sa_mask);
	segf_action.sa_sigaction = signal_callback_handler;
	if(sigaction(SIGSEGV, &segf_action, NULL) != 0){
		ERROR("did not register seg fault handler\n");
	}

	// how many bits to represent a page?
	size_t num_bits = (size_t)log2((double)RMEM_PG_SIZE);

	// create a mask for the bottom 'num_bits' of the total 64 bit address space
	RMEM_PAGE_BASE_MASK = (ULONG_MAX >> num_bits) << num_bits;

	//setup an address cache
	local_mem_block_cache.clear();
	local_mem_block_cache.init(memblk_cache_size);

	PRINTF("RMEM_INIT: init dpdk\n");
	// TEMPORARY
	dpdk_main(28 /*mem_size_bits*/, 8 /*page_size_bits*/);
	PRINTF("RMEM_INIT: init dpdk .... DONE\n");

	// clear the accounting
	data_map.clear();
	addr_space_map.clear();

	// get a unique id for this node, and start heartbeat
	generate_uuid(uuid);
	client 	= new Heartbeat(uuid);
	client->start_daemon();

	/*
	// record the slaves for later use
	for(size_t i = 0 ; i < num_slaves; i++){
        slaves.push_back(slave_s[i]);

        // we also establish heartbeats to the slaves
        std::string tmp_ip;
        tmp_ip.assign((char*)slave_s[i].ip_addr, IP_ADDR_LEN_BYTES);
		client->add_slave(tmp_ip);
	}
	*/

	// read the machines from a file
	std::ifstream machines_stream(machines_csv_file_path);
	std::vector<std::string> vals;

	vals = getNextLineAndSplitIntoTokens(machines_stream);
	while(vals.size() == 2) {
		//printf("%s, %s\n", 	vals.at(0).c_str(),	vals.at(1).c_str());

		Location tmp_l;
		strcpy((char*)&tmp_l.ip_addr, vals.at(0).c_str());
		sscanf(vals.at(1).c_str(), "%2X:%2X:%2X:%2X:%2X:%2X", 
					        (unsigned int *)&tmp_l.mac_addr[0], 
					        (unsigned int *)&tmp_l.mac_addr[1], 
					        (unsigned int *)&tmp_l.mac_addr[2], 
					        (unsigned int *)&tmp_l.mac_addr[3], 
					        (unsigned int *)&tmp_l.mac_addr[4], 
					        (unsigned int *)&tmp_l.mac_addr[5]);
		
		printf("Register machine: ");location_print(tmp_l);

		slaves.push_back(tmp_l);
		client->add_slave(vals.at(0));

		vals = getNextLineAndSplitIntoTokens(machines_stream);				
	}

//	processing_thread = thread(process_pages);
	
}
//------------------------------------------------------------------------------
void rum_flush_cache(){
	list<Addr> cache = local_mem_block_cache.get_list();
	list<Addr>::iterator  					cache_itter;
	Addr 									stale_blk;
	Addr 									base_ptr;
	map<Addr, Addr>::iterator 				low;
	map<Addr, vector<Location>*>::iterator 	loc_iter;
	Addr 									offset;
	map<Addr, ACT_STRUCT>::iterator 		data_it;
	map<Addr, char>::iterator 				dirty_it;

    int count = 0;
	for(cache_itter = cache.begin() ; cache_itter != cache.end() ; cache_itter++){
		stale_blk = *cache_itter;
		PRINTF("FLUSH :[%d] %lx\n", count++, stale_blk);

		// turn off protection
	 	mprotect((void*)stale_blk, RMEM_PG_SIZE, PROT_READ | PROT_WRITE);

		low = addr_space_map.lower_bound((Addr)stale_blk);
		if(low == addr_space_map.end()){
			base_ptr = addr_space_map.rbegin()->second;
		}
		else{
			base_ptr = low->second;
		}
		offset = stale_blk - base_ptr;

		// send this offset!
		//PRINTF("FLUSH\n");

		if((data_it = data_map.find((Addr)base_ptr)) == data_map.end()){
			ERROR("RUM_TRAP: can't find the pointer location (data_map)\n");
		}
		//dump_act(data_it->second);

		if((dirty_it = dirty_map.find((Addr)stale_blk)) != dirty_map.end()){
			if(dirty_it->second){
				dpdk_write(data_it->second.replicas->at(0), data_it->second.remote, offset, stale_blk, RMEM_PG_SIZE);		
				dirty_it->second = 0;

				// should probably remove this now..
			}
		}
		

		// flush the stale page to remote
		//PRINTF("=== stale blk 0x%lx\n", (long)stale_blk);

		// flushed, so munlock from local RAM
		if (munlock((void *)stale_blk, RMEM_PG_SIZE) != 0) {
			perror("munlock"); // print error but continue
			printf("address 0x%lx", stale_blk);
		}
	}

	// note: we do not turn protection back on
	local_mem_block_cache.clear();
	//PRINTF("FLUSH: cache size %ld\n", local_mem_block_cache.get_list().size());
}
//------------------------------------------------------------------------------
void rum_end(void){
	//PRINTF("RUM_END\n");
	map<Addr, Addr>::iterator low;
	map<Addr, vector<Location>*>::iterator 	loc_iter;
	map<Addr, size_t>::iterator ptr_it;
	map<Addr, ACT_STRUCT>::iterator 		data_it;

	// we should flush everything to the cash
	//for (Addr elem: dirty_pages) {
		//PRINTF("FLUSH : 0x%lx\n", elem);
	    //dest = addr_map.at(memblk_base);
		//dpdk_write(dest.at(0), stale_blk, RMEM_PG_SIZE);
	//}

	rum_flush_cache();

	std::lock_guard<std::mutex> lock(memory_mutex);

	TODO("data_map\n");
	debug_dump_lists();

	/*
	// now write out normally, without going through the fault mechanism (cache)
	for(data_it = data_map.begin(); data_it != data_map.end(); data_it++){
		ACT_STRUCT *d = &(data_it->second);

		// turn off protection
		mprotect((void*)d->start, d->size, PROT_READ | PROT_WRITE);

		TODO("Should optimise this per page, not all memory\n");
		if(d->dirty_bit){
			TODO("this is a candidate for bulk move\n");
			for(size_t i = 0 ; i < d->size ; i += RMEM_PG_SIZE){
				dpdk_write(d->replicas->at(0), d->remote, i, d->start, RMEM_PG_SIZE);
			}
			d->dirty_bit = 0;
		}

		// if this was in the cache, purge it
		local_mem_block_cache.purge(d->start, d->start + d->size);

		// free
		munmap((void*)d->start, d->size);
	}
	*/

	// clear the cache
	local_mem_block_cache.clear();
	data_map.clear();
	addr_space_map.clear();

	//pointer_size_map.clear();

	// stop the heartbeats
	client->stop_daemon();	

	//PRINTF("RUM_END: cache size %ld\n", local_mem_block_cache.get_list().size());
	//PRINTF("RUM_END......END\n");	

	
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
// debug 
//static bool trigger = false;
//------------------------------------------------------------------------------	
void* rum_alloc(int size){
    //PRINTF("RUM_ALLOC: %ld bytes\n", size);
#if RAW
    return malloc(size);
#endif
	//PRINTF("RUM_ALLOC: size\n");
	// chose somewhere to send the data
	char success = 0;
	void* ptr;
	char* temp;
	size_t alloc_size;
	size_t index;
	vector<Location>* tmp_loc = new vector<Location>();

	std::lock_guard<std::mutex> lock(memory_mutex);

	if(size <= 0){
		PRINTF("RUM_ALLOC: must allocate more than 0 bytes\n");
		return NULL;
	}

	// Allocate the minimum num of pages for this memory
	// (this all needs to be page aligned...)
	if(size <= RMEM_PG_SIZE){
		alloc_size = RMEM_PG_SIZE;
	}
	else{
		if((size % RMEM_PG_SIZE) == 0){
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE);	
		}
		else{
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE) + RMEM_PG_SIZE;
		}
	}

	// go contact the servers, and ask what information they can give
	// random choice for just now
	index = rand() % slaves.size();
	for( size_t i = 0;i < slaves.size(); i++, index = (index++ % slaves.size()))
	{

		//PRINTF("RUM_ALLOC: slave:  "); location_print(slaves.at(index));
		// ask for how much space is free?
		if((ptr = (void*)dpdk_alloc(slaves.at(index), alloc_size, uuid)) != NULL){
			// fsm is broken for now
			//mem_init(ptr, ptr + req_size);

			// record the location of the pointer
			tmp_loc->push_back(slaves.at(index));
			
			//PRINTF("RUM_ALLOC: got %ld bytes @ %lx\n", alloc_size, ADDR(ptr));
			success = 1;
			// we found space, so break
			break;
		}
		else{
			ERROR("RUM_ALLOC: could not allocate %ld bytes\n", alloc_size);
		}
	}

	// there is no space left in cluster memory
	if(!success){
		
		return NULL;
	}
	// now we have space, make some local cache

	// open a file to write zeros
	int fd = open("/dev/zero", O_RDONLY);
	if((temp = (char*) mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
		ERROR("Failed to map memory : %ld bytes\n", alloc_size); 
		ERROR("rum_alloc bytes %ld\n", rum_alloc_bytes);
		ERROR("rum_free bytes %ld\n",  rum_free_bytes);
		ERROR("total in use %ld bytes\n",  rum_alloc_bytes - rum_free_bytes);
		ERROR("----------------------\n");
		ERROR("mmap_alloc bytes %ld\n", mmap_alloc_bytes);
		ERROR("mmap_free bytes %ld\n",  mmap_free_bytes);
		ERROR("total in use %ld bytes\n",  mmap_alloc_bytes - mmap_free_bytes);
        
        /*
        std::ifstream fp("/proc/self/status");
        std::string line;
        while(std::getline(fp, line)){
            ERROR("\t%s\n", line.c_str());
        }
        perror("mmap");
		*/
        //exit(EXIT_FAILURE);
        // just take the first entry in the location list
		dpdk_dealloc(tmp_loc->at(0), (Addr)ptr, uuid);
		delete tmp_loc;
		return NULL;
	}

	// accouting
	mmap_alloc_bytes += alloc_size;

	//protect the pages that are returned so that we will trip the handler
  	if(mprotect (temp, alloc_size, PROT_NONE)){
  		ERROR("there was an error protecting the page :%s\n", strerror(errno));
                perror("mprotect");
                exit(EXIT_FAILURE);
  	}

	close(fd);

	//PRINTF("RUM_ALLOC: alloc %ld bytes @ 0x%lx (0x%lx)\n", alloc_size, ADDR(temp), ADDR(ptr));
	ACT_STRUCT tmp_entry;
	//tmp_entry.dirty_bit 	= 0;
	tmp_entry.start 		= (Addr)temp;
	tmp_entry.end   		= (Addr)(temp + alloc_size - 1);
	tmp_entry.remote 		= (Addr)ptr;
	tmp_entry.size 			= size;
	tmp_entry.alloc_size 	= alloc_size;
	tmp_entry.has_hash      = 0;
	tmp_entry.replicas 		= tmp_loc;
	data_map.insert({(Addr)temp, tmp_entry});

	rum_alloc_bytes += size;

	// because we count from 0th position
	addr_space_map.insert({tmp_entry.end, tmp_entry.start});

	
	return (void*)temp;
}
//------------------------------------------------------------------------------
void* rum_alloc(int size, Location location){
	//PRINTF("RUM_ALLOC: size, location\n");
	// chose somewhere to send the data
	void* ptr;
	char* temp;
	size_t alloc_size;
	vector<Location>* tmp_loc = new vector<Location>();

	if(size <= 0){
		return NULL;
	}
	// Allocate the minimum num of pages for this memory
	// (this all needs to be page aligned...)
	if(size <= RMEM_PG_SIZE){
		alloc_size = RMEM_PG_SIZE;
	}
	else{
		if((size % RMEM_PG_SIZE) == 0){
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE);	
		}
		else{
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE) + RMEM_PG_SIZE;
		}
	}

	std::lock_guard<std::mutex> lock(memory_mutex);

	// go contact the servers, and ask what information they can give
    PRINTF("RUM_ALLOC: location:  "); location_print(location);
	// ask for how much space is free?
	if((ptr = (void*)dpdk_alloc(location, alloc_size, uuid)) != NULL){
		// fsm is broken for now
		//mem_init(ptr, ptr + req_size);
		
		// record the location of the pointer
		tmp_loc->push_back(location);
		PRINTF("RUM_ALLOC: got %ld bytes @ %lx\n", alloc_size, ADDR(ptr));
	}
	else{
        ERROR("RUM_ALLOC: could not allocate %ld bytes @  ", alloc_size); location_print(location);
		
		return NULL;
	}

	// now we have space, make some local cache

	// open a file to write zeros
	int fd = open("/dev/zero", O_RDONLY);
	if((temp = (char*) mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
		ERROR("Failed to map memory\n");
		TODO("should free all memory so far\n");
		
		return NULL;
	}

	// accouting
	mmap_alloc_bytes += alloc_size;

	//protect the pages that are returned so that we will trip the handler
  	if(mprotect (temp, alloc_size, PROT_NONE)){
  		ERROR("there was an error protecting the page :%s\n", strerror(errno));
  		TODO("should free all memory so far\n");
  		
  		return NULL;
  	}

	close(fd);

	PRINTF("RUM_ALLOC : alloc %ld bytes @ 0x%lx\n", alloc_size, (Addr)temp);

	ACT_STRUCT tmp_entry;
	//tmp_entry.dirty_bit 	= 0;
	tmp_entry.start 		= (Addr)temp;
	tmp_entry.end   		= (Addr)(temp + alloc_size - 1);
	tmp_entry.remote 		= (Addr)ptr;
	tmp_entry.size 			= size;
	tmp_entry.alloc_size 	= alloc_size;
	tmp_entry.has_hash      = 0;
	tmp_entry.replicas = tmp_loc;
	data_map.insert({(Addr)temp, tmp_entry});

	rum_alloc_bytes += size;

	// because we count from 0th position
	addr_space_map.insert({tmp_entry.end, tmp_entry.start});

	

	return (void*)temp;
}
//------------------------------------------------------------------------------
void* rum_alloc(int size, HASH_TYPE hash){
	//PRINTF("RUM_ALLOC: size, hash\n");
	// chose somewhere to send the data
	char success = 0;
	void* ptr;
	char* temp;
	size_t alloc_size;
	size_t index;
	vector<Location>* tmp_loc = new vector<Location>();

	if(size <= 0){
		return NULL;
	}

	// Allocate the minimum num of pages for this memory
	// (this all needs to be page aligned...)
	if(size <= RMEM_PG_SIZE){
		alloc_size = RMEM_PG_SIZE;
	}
	else{
		if((size % RMEM_PG_SIZE) == 0){
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE);	
		}
		else{
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE) + RMEM_PG_SIZE;
		}
	}

	std::lock_guard<std::mutex> lock(memory_mutex);

	// go contact the servers, and ask what information they can give
	// random choice for just now
	index = rand() % slaves.size();
	for( size_t i = 0;i < slaves.size(); i++, index = (index++ % slaves.size()))
	{
		PRINTF("RUM_ALLOC: slave:  "); location_print(slaves.at(index));
		// ask for how much space is free?
		if((ptr = (void*)dpdk_alloc(slaves.at(index), alloc_size, hash)) != NULL){
			// fsm is broken for now
			//mem_init(ptr, ptr + req_size);
			// record the location of the pointer
			tmp_loc->push_back(slaves.at(index));
			PRINTF("RUM_ALLOC: got %ld bytes @ %lx\n", alloc_size, ADDR(ptr));
			success = 1;
			// we found space, so break
			break;
		}
		else{
			ERROR("RUM_ALLOC: could not allocate %ld @ %s\n", alloc_size, TO_STR(hash));
		}
	}

	// there is no space left in cluster memory
	if(!success){
		
		return NULL;
	}
	// now we have space, make some local cache

	// open a file to write zeros
	int fd = open("/dev/zero", O_RDONLY);
	if((temp = (char*) mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
		ERROR("Failed to map memory\n");
		TODO("free memory so far\n");
		
		return NULL;
	}

	// accouting
	mmap_alloc_bytes += alloc_size;

	//protect the pages that are returned so that we will trip the handler
  	if(mprotect (temp, alloc_size, PROT_NONE)){
  		ERROR("there was an error protecting the page :%s\n", strerror(errno));
  		TODO("free memory so far\n");
		
		return NULL;
  	}

	close(fd);

	//PRINTF("RUM_ALLOC : alloc %ld bytes @ 0x%lx\n", alloc_size, (Addr)temp);

	ACT_STRUCT tmp_entry;
	//tmp_entry.dirty_bit 	= 0;
	tmp_entry.start 		= (Addr)temp;
	tmp_entry.end   		= (Addr)(temp + alloc_size);
	tmp_entry.remote 		= (Addr)ptr;
	tmp_entry.size 			= size;
	tmp_entry.alloc_size 	= alloc_size;
	tmp_entry.has_hash      = 1;
	tmp_entry.h 			= hash;
	tmp_entry.replicas = tmp_loc;
	data_map.insert({(Addr)temp, tmp_entry});

	rum_alloc_bytes += size;

	// because we count from 0th position
	addr_space_map.insert({tmp_entry.end, tmp_entry.start});

	

	return (void*)temp;
}
//------------------------------------------------------------------------------
void* rum_alloc(int size, HASH_TYPE hash, Location l){
	//PRINTF("RUM_ALLOC: size, hash, location\n");
	// chose somewhere to send the data
	void* ptr;
	char* temp;
	size_t alloc_size;
	vector<Location>* tmp_loc = new vector<Location>();
        
	if(size <= 0){
		return NULL;
	}

#if 0
	// DEBUG: push the data to random locations, to test the address mechanism
	size_t index = rand() % slaves.size();
        l = slaves->at(index);
#endif
	// Allocate the minimum num of pages for this memory
	// (this all needs to be page aligned...)
	if(size <= RMEM_PG_SIZE){
		alloc_size = RMEM_PG_SIZE;
	}
	else{
		if((size % RMEM_PG_SIZE) == 0){
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE);	
		}
		else{
			alloc_size = ((size / RMEM_PG_SIZE) * RMEM_PG_SIZE) + RMEM_PG_SIZE;
		}
	}

	std::lock_guard<std::mutex> lock(memory_mutex);

	// go contact the servers, and ask what information they can give
	PRINTF("RUM_ALLOC: slave:  "); location_print(l);
        
	// ask for how much space is free?
	if((ptr = (void*)dpdk_alloc(l, alloc_size, hash)) == NULL){
		ERROR("RUM_ALLOC: could not allocate %ld @ %s\n", alloc_size, TO_STR(hash));
		
		return NULL;
	}
	PRINTF("RUM_ALLOC: got %ld bytes @ %lx\n", alloc_size, ADDR(ptr));
	tmp_loc->push_back(l);
	

	// now we have space, make some local cache

	// open a file to write zeros
	int fd = open("/dev/zero", O_RDONLY);
	if((temp = (char*) mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
		ERROR("Failed to map memory\n");
		TODO("free memory so far\n");
		
		return NULL;
	}

	// accouting
	mmap_alloc_bytes += alloc_size;

	//protect the pages that are returned so that we will trip the handler
  	if(mprotect (temp, alloc_size, PROT_NONE)){
  		ERROR("there was an error protecting the page :%s\n", strerror(errno));
  		TODO("free memory so far\n");
		
		return NULL;
  	}

	close(fd);

	//PRINTF("RUM_ALLOC : alloc %ld bytes @ 0x%lx\n", alloc_size, (Addr)temp);

	ACT_STRUCT tmp_entry;
	//tmp_entry.dirty_bit 	= 0;
	tmp_entry.start 		= (Addr)temp;
	tmp_entry.end   		= (Addr)(temp + alloc_size - 1);
	tmp_entry.remote 		= (Addr)ptr;
	tmp_entry.size 			= size;
	tmp_entry.alloc_size 	= alloc_size;
	tmp_entry.has_hash      = 1;
	tmp_entry.h 			= hash;
	tmp_entry.replicas = tmp_loc;
	data_map.insert({(Addr)temp, tmp_entry});

	rum_alloc_bytes += size;

	// because we count from 0th position
	addr_space_map.insert({tmp_entry.end, tmp_entry.start});

	

	return temp;
}
//------------------------------------------------------------------------------
// don't need locks in this one as no accounting manipulation
void* rum_calloc (int num, int size){
	//PRINTF("RUM_CALLOC: %ld elemenets of %ld bytes\n", num, size);
	void *new_guy;

#if RAW
    return calloc(num, size);
#endif
	if((new_guy = rum_alloc(num * size)) == NULL){
		return NULL;
	}

	return new_guy;
}
//------------------------------------------------------------------------------
void* rum_realloc (void* ptr, int size){
	//PRINTF("RUM_REALLOC %lx ( %ld bytes)\n", ADDR(ptr), size);
        
#if RAW
    return realloc(ptr, size);
#endif
    if(size <= 0){
    	return NULL;
    }
	else if(ptr == NULL){
        //printf("\t");
        return rum_alloc(size);
	}

	

	map<Addr, ACT_STRUCT>::iterator data_it;
	map<Addr, size_t>::iterator size_iter;
	char 						*new_guy;
	size_t 						limit;

	//std::lock_guard<std::mutex> lock(memory_mutex);
	memory_mutex.lock();

	// find the size actually allocated
	if((data_it = data_map.find(ADDR(ptr))) == data_map.end()){
		ERROR("rum_realloc: can't find size of ptr %lx\n", ADDR(ptr));
		debug_dump_lists();
		memory_mutex.unlock();
		//rum_free(new_guy);
		return NULL;
	}
	
	// allocation unit is a page, so nothing to do
	if(size < RMEM_PG_SIZE && data_it->second.alloc_size <= (size_t)RMEM_PG_SIZE){
		memory_mutex.unlock();
		return ptr;
	}

	// copy the lesser of the values
	limit = (size < (int)data_it->second.size) ? size : data_it->second.size;

	// can't mix reentrant and lock_guard
	memory_mutex.unlock();
	// printf("\t");
	if((new_guy = (char*)rum_alloc(size)) == NULL){
		return NULL;
	}

	// feels like this should be locked...
    memcpy(new_guy, ptr, limit);

    // now tidy the older pointer
    rum_free(ptr);

    //PRINTF("RUM_REALLOC: new ptr %lx ( %ld bytes + page rounding)\n", ADDR(new_guy),  size);
	return (void*)new_guy;
}
//------------------------------------------------------------------------------
void* rum_get(HASH_TYPE hash, Location l){
	uint64_t 	alloc_size;
	Addr 		remote_ptr;
	char 		*local_ptr;
	vector<Location>* tmp_loc = new vector<Location>();

	//PRINTF("rum_get: hash '%s'\n", hash.c_str());

	std::lock_guard<std::mutex> lock(memory_mutex);

	// go get the pointer by hash
	dpdk_get_hash(l, hash, &alloc_size, (char*)&remote_ptr);

	if(alloc_size == 0){
		//PRINTF("rum_get: memory for hash '%s' not found\n", hash.c_str());
		
		return NULL;
	}

	// it is there, make a local cache
	// open a file to write zeros
	int fd = open("/dev/zero", O_RDONLY);
	if((local_ptr = (char*) mmap(NULL, alloc_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0)) == MAP_FAILED){
		ERROR("Failed to map memory\n");
		
		return NULL;
	}

	// accouting
	mmap_alloc_bytes += alloc_size;

	//protect the pages that are returned so that we will trip the handler
  	if(mprotect (local_ptr, alloc_size, PROT_NONE)){
  		ERROR("there was an error protecting the page :%s\n", strerror(errno));
  		munmap(local_ptr, alloc_size);
  		return NULL;
  	}

	close(fd);

	tmp_loc->push_back(l);

	//PRINTF("rum_get: alloc %ld bytes @ 0x%lx\n", alloc_size, (Addr)local_ptr);
	ACT_STRUCT tmp_entry;
	//tmp_entry.dirty_bit 	= 0;
	tmp_entry.start 		= (Addr)local_ptr;
	tmp_entry.end   		= (Addr)(local_ptr + alloc_size - 1);
	tmp_entry.remote 		= (Addr)remote_ptr;
	TODO("should this be some special case??\n");
	tmp_entry.size 			= alloc_size; 
	tmp_entry.alloc_size 	= alloc_size;
	tmp_entry.has_hash      = 1;
	tmp_entry.h 			= hash;
	//memcpy(&tmp_entry.h, hash, sizeof(HASH_TYPE));
	tmp_entry.replicas 		= tmp_loc;
	data_map.insert({(Addr)local_ptr, tmp_entry});

	rum_alloc_bytes += tmp_entry.size;

	// because we count from 0th position
	addr_space_map.insert({tmp_entry.end, tmp_entry.start});
	

	//PRINTF("rum_get: return the ptr\n");
	return (void*)local_ptr;
}
//------------------------------------------------------------------------------
void rum_free(void* ptr){
	//PRINTF("RUM_FREE %lx \n", ADDR(ptr));
#if RAW
    free(ptr);
    return;
#endif
    
	if(ptr == NULL){ return; }

	map<Addr, ACT_STRUCT>::iterator data_it;
	ACT_STRUCT 						d;

	std::lock_guard<std::mutex> lock(memory_mutex);

	if((data_it = data_map.find(ADDR(ptr))) == data_map.end()){
		ERROR("RUM_FREE: can't find the ACT_STRUCT\n");
		return;
	}
	else{
		//dump_act(data_it->second);
		d = data_it->second;
	}

	//PRINTF("RUM_FREE: free %ld bytes @ 0x%lx\n", ptr_iter->second, ADDR(ptr));

	// need to remove any pages from the cache incase
	local_mem_block_cache.purge(ADDR(ptr), ADDR(ptr) + d.size);
    //local_mem_block_cache.dump();

	// just take the first entry in the location list
	dpdk_dealloc(d.replicas->at(0), (Addr)d.remote, uuid);

	// turn off protection
	mprotect(ptr, d.size, PROT_READ | PROT_WRITE);

	// free from vm
	munmap(ptr, d.size);

	// tidy accouting
	rum_free_bytes += d.size;
	mmap_free_bytes += d.size;
	addr_space_map.erase(d.end);
	data_map.erase(ADDR(ptr));
}
//------------------------------------------------------------------------------
void rum_free(void* ptr, HASH_TYPE h){
#if RAW
    free(ptr);
    return;
#endif
    
	if(ptr == NULL){ return; }

	//map<Addr, size_t>::iterator 			ptr_iter;
	//map<Addr, vector<Location>*>::iterator 	loc_iter;
	map<Addr, ACT_STRUCT>::iterator 		data_it;
	ACT_STRUCT d;

	std::lock_guard<std::mutex> lock(memory_mutex);

	if((data_it = data_map.find(ADDR(ptr))) == data_map.end()){
		ERROR("RUM_FREE: can't find the ACT_STRUCT\n");
		
		return;
	}
	else{
		//dump_act(data_it->second);
		d = data_it->second;
	}

	//PRINTF("RUM_FREE: free %ld bytes @ 0x%lx\n", ptr_iter->second, ADDR(ptr));

	// need to remove any pages from the cache incase
	local_mem_block_cache.purge(ADDR(ptr), ADDR(ptr) + d.size);
    //local_mem_block_cache.dump();

	// just take the first entry in the location list
	TODO("Replica managment\n");
	dpdk_dealloc(d.replicas->at(0), h);

	// turn off protection
	mprotect(ptr, d.size, PROT_READ | PROT_WRITE);

	// free
	munmap(ptr, d.size);

	// tidy accouting
	mmap_free_bytes += d.size;
	rum_free_bytes += d.size;
	addr_space_map.erase(d.end);
	data_map.erase(ADDR(ptr));
}
//------------------------------------------------------------------------------
void rum_free(Location l, HASH_TYPE h){
	map<Addr, ACT_STRUCT>::iterator 		data_it;
	ACT_STRUCT d;

	std::lock_guard<std::mutex> lock(memory_mutex);

	//bool found = false;
	for(data_it = data_map.begin() ; data_it != data_map.end() ; data_it++){
		d = data_it->second;

		if(strcmp(HASH_STR(d.h), HASH_STR(h)) == 0){
			//found = true;
			//dump_act(d);

			// need to remove any pages from the cache incase
			local_mem_block_cache.purge(ADDR(d.start), ADDR(d.start) + d.size);

			// turn off protection
			mprotect((void*)d.start, d.size, PROT_READ | PROT_WRITE);

			// free
			munmap((void*)d.start, d.size);

			// accounting
			mmap_free_bytes += d.size;
			rum_free_bytes += d.size;
			addr_space_map.erase(d.end);
			data_map.erase(data_it);
		}
	}

	//if(!found){
	//	ERROR("RUM_FREE: could not find data on hash '%s'\n", HASH_STR(h));
	//}
	//else{
		//PRINTF("RUM_FREE: free %ld bytes @ 0x%lx\n", ptr_iter->second, ADDR(ptr));
		// now free the data remotely
		dpdk_dealloc(l, h);
	//}

	return;
/*
	for(hash_iter = slave_ptr_hash_map.begin() ; hash_iter != slave_ptr_hash_map.end() ; hash_iter++){
		// did we ever see this locally?
		if(strcmp(HASH_STR(hash_iter->second), HASH_STR(h)) == 0){
			
			void* ptr = (void*)hash_iter->first;

			if((ptr_iter = pointer_size_map.find(ADDR(ptr))) != pointer_size_map.end()){
				PRINTF("RUM_FREE: the pointer's size'\n");
			}

			if((loc_iter = addr_map.find(ADDR(ptr))) != addr_map.end()){
				PRINTF("RUM_FREE: the pointer location\n");
			}

			// need to remove any pages from the cache incase
			local_mem_block_cache.purge(ADDR(ptr), ADDR(ptr) + ptr_iter->second);

			// get the allocated length, and remove the pointer
			size_t size = ptr_iter->second;
			pointer_size_map.erase(ADDR(ptr));

			delete addr_map[ADDR(ptr)];
			addr_map.erase(ADDR(ptr));

			slave_ptr_map.erase(ADDR(ptr));
		        
		    addr_space_map.erase(ADDR(ptr) + ptr_iter->second - 1);

			// turn off protection
			mprotect(ptr, size, PROT_READ | PROT_WRITE);

			// if something to write, flush
			if(dirty_pages.count(ADDR(ptr) > 0)){
				//vector<Location> dest = addr_map.at(ADDR(ptr));
				//dpdk_write(dest.at(0), ADDR(ptr), RMEM_PG_SIZE);
				dirty_pages.erase(ADDR(ptr));
			}

			// remove the pointer from the map
			//vector<Location>* dests = addr_map.at(ADDR(ptr));
			//delete dests;
			//addr_map.erase(ADDR(ptr));

			rum_free_bytes += size;

			// free
			munmap(ptr, size);
			break;
		}
	}
	*/
}
//------------------------------------------------------------------------------
// Should we have a timed flush?
void invalidate(){
	usleep(10);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/*
void signal_callback_handler2(int signum, siginfo_t *info, void *extra){
    char 						write = 0;
	map<Addr, Addr>::iterator 	low;
	Addr 						base_ptr;
	map<Addr, vector<Location>*>::iterator 	loc_iter;
	Addr 									offset;

    // get the access type
    if(((ucontext_t*)extra)->uc_mcontext.gregs[REG_ERR] & 0x2){
        write = 1;
    } else {
        write = 0;
    }

    // get the starting addr of the page containing this memory
	Addr memblk_base = (Addr)info->si_addr & RMEM_PAGE_BASE_MASK;
	//PRINTF("TRAP: %s @ 0x%lx (0x%lx)\n", (write == 1?"write":"read"), (Addr) info->si_addr, (Addr) memblk_base);

	//find the original malloc'd pointer
	low = addr_space_map.lower_bound((Addr)info->si_addr);

	// Check this is not an actual pointer error
    // (in the map first is the end addr, second is the start)
    if(low == addr_space_map.end()){
    	if((Addr)info->si_addr > addr_space_map.rbegin()->first
    		|| (Addr)info->si_addr < addr_space_map.rbegin()->second){
            //usleep(1000000);
    		ERROR("TRAP: [wrap] should never see this, you probably mis-accessed some memory : %lx\n", (Addr)info->si_addr);
                debug_dump_lists();
                exit(EXIT_FAILURE);
	    }
        base_ptr = addr_space_map.rbegin()->second;
    }
    else{
    	if((Addr)info->si_addr < low->second){
            //usleep(1000000);
    		ERROR("TRAP: should never see this, you probably mis-accessed some memory : %lx\n", (Addr)info->si_addr);
                debug_dump_lists();
                exit(EXIT_FAILURE);
    	}
    	base_ptr = low->second;
    }

    // turn off protection, so we can access the page
	mprotect((void*)memblk_base, RMEM_PG_SIZE, PROT_READ | PROT_WRITE);

	// Write to the page to obtain a private copy.
	// if a write, this 0 is overwritten. if a read, this 0 is overwritten
	*((char *)memblk_base) = *((char *)memblk_base); 

	// lock the page into RAM
	if(mlock((void*)memblk_base, RMEM_PG_SIZE) != 0){
		ERROR("TRAP: mlock @ 0x%lx : %s\n", memblk_base, strerror(errno));
	}

	// have to handle any reads immediately
	if(!write){
		// we only get here if the page was locked by a previous flush, so sync
		// this read will be come useful if we move machine

		offset = memblk_base - base_ptr;

		// can read based on hash or ptr
		// there should be error handling here to try alternate devices
		if((loc_iter = addr_map.find((Addr)base_ptr)) == addr_map.end()){
			ERROR("TRAP: read can't find the pointer location\n");
		}

		dpdk_read(loc_iter->second->at(0), slave_ptr_map[base_ptr], offset, memblk_base, RMEM_PG_SIZE);
	}

	// put the data in the buffer to be processed
    memory_q.enqueue({write, ADDR(info->si_addr), base_ptr});
}
*/
//------------------------------------------------------------------------------
#if 1
//https://stackoverflow.com/questions/43033177/when-catching-sigsegv-from-within-how-to-known-the-kind-of-invalid-access-invol
void signal_callback_handler(int signum, siginfo_t *info, void *extra){
	//PRINTF("TRAP: caught signal %d\n", signum);
	vector<Location> 						dest;
	char 									write = 0;
	Addr 									stale_blk;
	Addr 									base_ptr;
	map<Addr, Addr>::iterator 				low;
	//map<Addr, vector<Location>*>::iterator 	loc_iter;
	Addr 									offset;
	map<Addr, ACT_STRUCT>::iterator 		data_it;
	map<Addr, char>::iterator 				dirty_it;
	ACT_STRUCT 								*d;

	if(((ucontext_t*)extra)->uc_mcontext.gregs[REG_ERR] & 0x2){
		write = 1;
	} else {
		write = 0;
	}
	
	// get the starting addr of the page containing this memory
	Addr memblk_base = (Addr)info->si_addr & RMEM_PAGE_BASE_MASK;
	//PRINTF("TRAP: %s @ 0x%lx (0x%lx)\n", (write == 1?"write":"read"), (Addr) info->si_addr, (Addr) memblk_base);

	//find the original malloc'd pointer
	low = addr_space_map.lower_bound((Addr)info->si_addr);

	// Check this is not an actual pointer error
    // (in the map first is the end addr, second is the start)
    if(low == addr_space_map.end()){
    	if((Addr)info->si_addr > addr_space_map.rbegin()->first
    		|| (Addr)info->si_addr < addr_space_map.rbegin()->second){
    		ERROR("TRAP: [wrap] should never see this, you probably mis-accessed some memory : %lx\n", (Addr)info->si_addr);
                debug_dump_lists();
                exit(EXIT_FAILURE);
	    }
        base_ptr = addr_space_map.rbegin()->second;
    }
    else{
    	if((Addr)info->si_addr < low->second){
    		ERROR("TRAP: should never see this, you probably mis-accessed some memory : %lx\n", (Addr)info->si_addr);
                debug_dump_lists();
                exit(EXIT_FAILURE);
    	}
    	base_ptr = low->second;
    }
#if 0
    //if(trigger)
    {
    	PRINTF("TRAP: %s @ 0x%lx (page_base 0x%lx, pointer_base0x%lx)\n", 
	    	(write == 1?"write":"read"), 
	    	(Addr) info->si_addr, 
	    	(Addr) memblk_base, 
	    	(Addr) base_ptr
		);
    	//usleep(10000000);
    }
#endif
	// turn off protection, so we can access the page
	mprotect((void*)memblk_base, RMEM_PG_SIZE, PROT_READ | PROT_WRITE);

	// Write to the page to obtain a private copy.
	*((char *)memblk_base) = *((char *)memblk_base); 

	// lock the page into RAM
	if(mlock((void*)memblk_base, RMEM_PG_SIZE) != 0){
		ERROR("TRAP: mlock @ 0x%lx (%d bytes): %s\n", memblk_base, RMEM_PG_SIZE, strerror(errno));
                //FILE* fp = fopen("/proc/self/status", "r");
                std::ifstream fp("/proc/self/status");
                std::string line;
                while(std::getline(fp, line)){
                    ERROR("\t%s\n", line.c_str());
                }
                exit(EXIT_FAILURE);
	}

	if((data_it = data_map.find((Addr)base_ptr)) == data_map.end()){
		ERROR("RUM_TRAP: read can't find the pointer location\n");
	}		
	else{
		d = &(data_it->second);
	}

	if(!write){
		// we only get here if the page was locked by a previous flush, so sync
		// this read will be come useful if we move machine
		//dest = addr_map.at(memblk_base);
		//PRINTF("READ\n");
		//if(local_mem_block_cache.contains(memblk_base)){
		//	PRINTF("UNFLUSHED!!!\n");
		//}

		offset = memblk_base - base_ptr;

		// can read based on hash or ptr
		// there should be error handling here to try alternate devices

		

		dpdk_read(d->replicas->at(0), d->remote, offset, memblk_base, RMEM_PG_SIZE);
		/*
		for(int i = 0 ; i < 280 ; i++){
			printf("[%d] %c (%d)\n", i, *((char*) memblk_base + i), *((char*) memblk_base + i));
		}
		*/
	}
	else{
		//PRINTF("RUM_TRAP: set dirty_map entry for %lx\n", memblk_base);
		dirty_map.insert({memblk_base, 1});
		//d->dirty_bit = 1;
		//dump_act(*d);
	}

	// if this new access pushes something out the local cache, flush to the
	// remote node. Cache should update ditry_pages....
	//TODO("enusre that cache will update ditry pages\n");
	if ((stale_blk = local_mem_block_cache.update(memblk_base)) != 0) {

		//local_mem_block_cache.dump();

		// calculate the offset (memblk - base_ptr)
		// no checking here, as should be checked upon cache entry
		low = addr_space_map.lower_bound((Addr)stale_blk);
		if(low == addr_space_map.end()){
			base_ptr = addr_space_map.rbegin()->second;
		}
		else{
			base_ptr = low->second;
		}
		offset = stale_blk - base_ptr;

		//debug_dump_lists();

		// send this offset!
		//PRINTF("FLUSH\n");

		// flush the stale page to remote
		//PRINTF("=== stale blk 0x%lx\n", (long)stale_blk);

		if((data_it = data_map.find((Addr)base_ptr)) == data_map.end()){
			ERROR("RUM_TRAP: write can't find the pointer location\n");
			ERROR("TRAP: %s @ 0x%lx (page_base 0x%lx, pointer_base 0x%lx)\n", 
		    	(write == 1?"write":"read"), 
		    	(Addr) info->si_addr, 
		    	(Addr) memblk_base, 
		    	(Addr) base_ptr
			);
		}		
		else{
			d = &(data_it->second);
		}

		if((dirty_it = dirty_map.find((Addr)stale_blk)) != dirty_map.end()){
			//PRINTF("=== stale blk 0x%lx is %s dirty\n", (long)stale_blk, (dirty_it->second ? "" : "not"));
			if(dirty_it->second){
				dpdk_write(d->replicas->at(0), d->remote, offset, stale_blk, RMEM_PG_SIZE);		
				//dirty_it->second = 0;

				// should probably remove this now..
				dirty_map.insert({stale_blk, 0});
			}
			//else{
			//	PRINTF("RUM_TRAP: dirty bit not set, do nothing : 0x%lx\n", stale_blk);
			//}
		}
		else{
			PRINTF("RUM_TRAP: could not find dirty_map entry for %lx\n", stale_blk);
			dump_dirty();
		}

		// flushed, so munlock from local RAM
		if (munlock((void *)stale_blk, RMEM_PG_SIZE) != 0) {
			perror("munlock"); // print error but continue
			printf("address 0x%lx", stale_blk);
		}

		// turn protection back on
		//printf(" turn on protection Addr 0x%lx\n", (long)stale_addr);
		mprotect ((void *)stale_blk, RMEM_PG_SIZE, PROT_NONE);
	}
	//PRINTF("TRAP:end\n");
} 
#endif
//------------------------------------------------------------------------------
/*
void process_pages(){
    MEM_ACCESS 								element;
    vector<Location> 						dest;
    Addr 									stale_blk;
    Addr 									stale_base_ptr;
    map<Addr, Addr>::iterator 				low;
    map<Addr, vector<Location>*>::iterator 	loc_iter;
    Addr 									offset;
    Addr 									memblk_base;


    for(;;){
        if(memory_q.try_dequeue(element)){

            // get the starting addr of the page containing this memory
            memblk_base = element.addr & RMEM_PAGE_BASE_MASK;
            //PRINTF("process_pages: %s @ 0x%lx (0x%lx)\n", (element.write == 1?"write":"read"), element.addr, (Addr) memblk_base);

            // if this new access pushes something out the local cache, flush to the
            // remote node. Cache should update ditry_pages....
            //TODO("enusre that cache will update ditry pages\n");
            if ((stale_blk = local_mem_block_cache.update(memblk_base)) != 0) {
                // calculate the offset (memblk - base_ptr)
                // no checking here, as should be checked upon cache entry
                low = addr_space_map.lower_bound(ADDR(stale_blk));
                if(low == addr_space_map.end()){
                        stale_base_ptr = addr_space_map.rbegin()->second;
                }
                else{
                        stale_base_ptr = low->second;
                }
                offset = stale_blk - stale_base_ptr;

                if((loc_iter = addr_map.find(ADDR(stale_base_ptr))) == addr_map.end()){
                        ERROR("process_pages: write can't find the pointer location\n");

                        debug_dump_lists();
                }

                dpdk_write(loc_iter->second->at(0), slave_ptr_map[stale_base_ptr], offset, stale_blk, RMEM_PG_SIZE);	

                // flushed, so munlock from local RAM
                if (munlock((void *)stale_blk, RMEM_PG_SIZE) != 0) {
                        perror("munlock"); // print error but continue
                        printf("address 0x%lx", stale_blk);
                }

                // turn protection back on
                mprotect ((void *)stale_blk, RMEM_PG_SIZE, PROT_NONE);
            }
        }
    }
}
*/ 
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void rum_stats(){
	;
}