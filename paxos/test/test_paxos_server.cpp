/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   test_paxos_server.cpp
 * Author: paul.harvey
 *
 * Created on 2018/10/05, 16:44
 */

#include <cstdlib>
#include "../paxos_instance.h"
#include "../node.h"
#include <vector>
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void callback_function(int value){
    PRINTF("CALLBACK :: %d\n", value);
}
//------------------------------------------------------------------------------
int main(int argc, char** argv) {

    if(argc < 2){
        printf("which server am i: 0, 1, 2 ????\n");
        return EXIT_SUCCESS;
    }
    int choice = atoi(argv[1]);
    printf("I am server %d\n", choice);
    

    /*
    std::shared_ptr<Node> alpha = std::make_shared<Node>("127.0.0.1", 45670);
    std::shared_ptr<Node> beta  = std::make_shared<Node>("127.0.0.1", 45680);
    std::shared_ptr<Node> gamma = std::make_shared<Node>("127.0.0.1", 45690);
    */

    std::shared_ptr<Node> alpha = std::make_shared<Node>("172.22.0.31", 45670);
    std::shared_ptr<Node> beta  = std::make_shared<Node>("172.22.0.32", 45670);
    std::shared_ptr<Node> gamma = std::make_shared<Node>("172.22.0.33", 45670);

    Node* nodes[] = {alpha.get(), beta.get(), gamma.get()};
   
    std::vector<std::shared_ptr<Node>> peers    = {alpha, beta, gamma};
    
    std::string filename     = "./paxos_" + std::to_string(choice) + ".bkk";
    
    paxos_instance *pi = new paxos_instance(peers.at(choice), peers, filename, &callback_function);
    
    pi->start();
    pi->daemon(*nodes[choice]);
    
    return EXIT_SUCCESS;
}

