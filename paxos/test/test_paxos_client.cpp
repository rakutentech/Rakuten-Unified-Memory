/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   test_paxos_client.cpp
 * Author: paul.harvey
 *
 * Created on 2018/10/05, 16:20
 */

#include <cstdlib>
#include <cstring>
#include <stdlib.h>

using namespace std;

#include "../paxos_instance.h"
#include "../node.h"
#include "../UDP_comms.h"

#include <thread>

#include <unistd.h>     // for the sleep
/*
 * 
 */
int main(int argc, char** argv) {

    if(argc < 2){
        printf("idiot: give me a value to share to the group\n");
        return EXIT_SUCCESS;
    }
    int         value   = atoi(argv[1]);
    std::string their_ip(argv[2]);

    Node master(their_ip, 45670);
    
    Node alpha("172.22.0.31", 45670);
    Node beta( "172.22.0.32", 45670);
    Node gamma("172.22.0.33", 45670);
    
    std::shared_ptr<Node> me = std::make_shared<Node>("127.0.0.1", 45670);
    
    list<Node*> peers    = {&alpha, &beta, &gamma};
    string filename     = "./paxos.bkk";
    
    //paxos_instance *pi = new paxos_instance(&me, peers, filename);
    
     for(Node* i : peers){
        PRINTF("PROPOSER: %s:%d\n", i->get_ip().c_str(), i->get_port());
     }
    
    // start the daemon
    //std::thread d_thread(&paxos_instance::daemon, pi, me);//me.get_ip(), me.get_port());
    
    //usleep(1000000);
    
    
    //pi->propose_value(std::to_string(value));
    
    //d_thread.join();
    
    
    printf("propose a value of %d\n", value);
    // send a propose value
    std::shared_ptr<Value> prep_val = std::make_shared<Value>(false, nullptr, std::to_string(value));
    
    std::shared_ptr<PAXOS_MSG> start = PAXOS_MSG::propose(prep_val);
    start->set_from_uid(me);
    
    size_t len = start->marshal_len();   
    char* buf  = (char*)malloc(len);
    start->marshal(buf, len);
    
    Node chosen_one = master;
    
    UDP_comms *c = new UDP_comms(chosen_one.get_ip(), chosen_one.get_port(), false);
    c->send(buf, len);
    
    free(buf);
    //delete start;
    
    return EXIT_SUCCESS;
}

