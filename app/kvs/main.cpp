/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   main.cpp
 * Author: paul.harvey
 *
 * Created on 2018/10/17, 14:00
 */

#include <cstdlib>
#include <memory>

#include "KVS.h"
#include "Key.h"
#include "Value.h"


#include <boost/filesystem.hpp>     // for the directory stuff
#include <vector>
#include <memory>               // for the shared pointers
#include <iostream>
#include <unistd.h>             // for the sleep

#include "util.h"

#include <Location.h>           // from RUM

using namespace std;

// given a path, return a string representing that path
struct path_leaf_string{
    string operator()(const boost::filesystem::directory_entry& entry) const {
        return entry.path().string();//.leaf().string();
    }
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
void check_value_marshalling(Value* a/*, Value* b*/){
    printf("check_value_marshalling\n");

    size_t a_marshal_len = a->marshal_len();
    //size_t b_marshal_len = b->marshal_len();

    char* a_marshal_buf  = (char*) malloc(a_marshal_len);
    //char* b_marshal_buf  = (char*) malloc(b_marshal_len);

    a->marshal(a_marshal_buf);
    //b->marshal(b_marshal_buf);

    Value *new_a = Value::demarshal(a_marshal_buf);
    //Value *new_b = Value::demarshal(b_marshal_buf);


    Value::compare_value(a, new_a);
    printf("=========================\n");
    //compare_value(b, new_b);


    //free(a_marshal_buf);
    //free(b_marshal_buf);

    //delete new_a;
    //delete new_b;

    //TODO("MEMORY MANAGMENT!!!\n");
    printf("----------\n");
}
//------------------------------------------------------------------------------
void test_kvs_access(string input_path, std::string machines_csv){
    
    ////////////////////////////
    // Create the Store
    ////////////////////////////
    shared_ptr<KVS> kvs = make_shared<KVS>(machines_csv);
    ////////////////////////////

    std::vector<std::string>  files;
    //map<KEY, std::shared_ptr<Value>> sanity_map;
    map<KEY, Value*> sanity_map;
    
    // get the files from the directory
    boost::filesystem::path p (input_path);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    transform(start, end, back_inserter(files), path_leaf_string());
    
    if(files.size()  == 0){
        printf("Unable to read the files\n");
        return;
    }
    
    printf("Read %ld files from %s\n", files.size(), input_path.c_str());
    
    // fill the store
    fstream src_file;
    size_t len;
    char* buf, *local_buf;
    for(uint64_t i = 0 ; i < files.size() ; i++){
        src_file.open(files.at(i), ios::in | ios::binary);
        if(!src_file.is_open()){
            printf("file %s not open\n", files.at(i).c_str());
            return;
        }
        // get the file size
        src_file.seekg(0, ios::end);
        len = src_file.tellg();
        src_file.seekg(0, ios::beg);
        
        buf         = (char*)malloc(len);
        local_buf   = (char*)malloc(len);
        
        // get the data
        src_file.read(buf, len);
        Value* v = new Value(buf, len);
        //shared_ptr<Value> v = make_shared<Value>(buf, len);
        
        // get the sanity data
        src_file.seekg(0, ios::beg);
        src_file.read(local_buf, len);
        Value* local_v = new Value(buf, len);
        ////////////////////////////
        // ADD THE DATA TO THE STORE
        ////////////////////////////
        kvs->add(files.at(i), v);

        sanity_map.insert({files.at(i), local_v});
        ////////////////////////////       
        src_file.close();


        // check the Value
        //check_value_marshalling(local_v);
        //Value::compare_value(v, local_v);
    }
    
    kvs->debug_dump_all_data();
    

    usleep(10000000);
    size_t num_of_access = files.size(); 
    //////////////////////////////////
    // Now do some Random Accesses
    //////////////////////////////////
    for(size_t i = 0 ; i < num_of_access ; i++){
        size_t index = i;//rand() % files.size();

        // these are shared pointers
        auto val_real = kvs->get(files.at(index));   
        auto val_sani = sanity_map[files.at(index)];

        Value::compare_value(val_real, val_sani);
    }
    
    //////////////////////////////////
    // Now do some Random Replacements
    //////////////////////////////////
    for(size_t i = 0 ; i < num_of_access ; i++){
        //b->put_data(/*Some key*/, /*some value*/);
    }

    //////////////////////////////////
    // Now do some Random Deletions
    //////////////////////////////////
    for(size_t i = 0 ; i < num_of_access ; i++){
         size_t index = i;//rand() % files.size();
        kvs->remove(files.at(index));
    }
}
//------------------------------------------------------------------------------
void test_kvs_recover_by_hash(string input_path, std::string machines_csv){
     ////////////////////////////
    // Create the Store
    ////////////////////////////
    shared_ptr<KVS> kvs_alpha = make_shared<KVS>(machines_csv);
    ////////////////////////////

    vector<string>  files;
    //map<KEY, std::shared_ptr<Value>> sanity_map;
    
    // get the files from the directory
    boost::filesystem::path p (input_path);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    transform(start, end, back_inserter(files), path_leaf_string());
    
    if(files.size()  == 0){
        printf("Unable to read the files\n");
        return;
    }
    
    printf("Read %ld files from %s\n", files.size(), input_path.c_str());
    
    // fill the store
    fstream src_file;
    size_t len;
    char* buf;
    for(uint64_t i = 0 ; i < files.size() ; i++){
        src_file.open(files.at(i), ios::in | ios::binary);
        if(!src_file.is_open()){
            printf("file %s not open\n", files.at(i).c_str());
            return;
        }
        // get the file size
        src_file.seekg(0, ios::end);
        len = src_file.tellg();
        src_file.seekg(0, ios::beg);
        
        buf         = (char*)malloc(len);
        
        printf("Read %ld bytes from %s\n", len, files.at(i).c_str());
        // get the data
        src_file.read(buf, len);
        Value* v = new Value(buf, len);
        //shared_ptr<Value> v = make_shared<Value>(buf, len);
        
        ////////////////////////////
        // ADD THE DATA TO THE STORE
        ////////////////////////////
        kvs_alpha->add(files.at(i), v);
        ////////////////////////////       
        src_file.close();
    }

    // close the kvs
    kvs_alpha->end();

    printf("TEST:: now make a new store and try to access the data\n");

    // now create a new store
    shared_ptr<KVS> kvs_beta = make_shared<KVS>(machines_csv);

    // now look for the files that we deposited
    for(uint64_t i = 0 ; i < files.size() ; i++){
        auto val = kvs_beta->get(files.at(i));

        if(val == NULL){
            ERROR("TEST_RECOVER_BY_HASH: could not find deposited file '%s'\n", files.at(i).c_str());
        }
        else{
            PRINTF("TEST_RECOVER_BY_HASH: SUCCESS '%s'\n", files.at(i).c_str());
        }
    }

    // now mkae sure that we can not find files which do not exist
    std::string fake_file("PAUL IS AMAZING!!!!");
    auto val = kvs_beta->get(fake_file);    
    if(val != NULL){
        ERROR("TEST_RECOVER_BY_HASH: could not find deposited file '%s'\n", fake_file.c_str());
    }
    else{
        PRINTF("TEST_RECOVER_BY_HASH: success, file not found\n");
    }

}   
//------------------------------------------------------------------------------
void test_kvs_remove_by_hash(string input_path, std::string machines_csv){
     ////////////////////////////
    // Create the Store
    ////////////////////////////
    shared_ptr<KVS> kvs_alpha = make_shared<KVS>(machines_csv);
    ////////////////////////////

    vector<string>  files;
    //map<KEY, std::shared_ptr<Value>> sanity_map;
    
    // get the files from the directory
    boost::filesystem::path p (input_path);
    boost::filesystem::directory_iterator start(p);
    boost::filesystem::directory_iterator end;
    transform(start, end, back_inserter(files), path_leaf_string());
    
    if(files.size()  == 0){
        printf("Unable to read the files\n");
        return;
    }
    
    printf("Read %ld files from %s\n", files.size(), input_path.c_str());
    
    // fill the store
    fstream src_file;
    size_t len;
    char* buf;
    for(uint64_t i = 0 ; i < files.size() ; i++){
        src_file.open(files.at(i), ios::in | ios::binary);
        if(!src_file.is_open()){
            printf("file %s not open\n", files.at(i).c_str());
            return;
        }
        // get the file size
        src_file.seekg(0, ios::end);
        len = src_file.tellg();
        src_file.seekg(0, ios::beg);
        
        buf         = (char*)malloc(len);
        
        printf("Read %ld bytes from %s\n", len, files.at(i).c_str());
        // get the data
        src_file.read(buf, len);
        Value* v = new Value(buf, len);
        //shared_ptr<Value> v = make_shared<Value>(buf, len);
        
        ////////////////////////////
        // ADD THE DATA TO THE STORE
        ////////////////////////////
        kvs_alpha->add(files.at(i), v);
        ////////////////////////////       
        src_file.close();
    }

    // close the kvs
    kvs_alpha->end();

    printf("TEST:: now remove some files\n");

    for(uint64_t i = 0 ; i < files.size() ; i++){
        kvs_alpha->remove(files.at(i));
    }

}
//------------------------------------------------------------------------------
/*
 * 
 */
int main(int argc, char* argv[]) {
    
    if(argc < 2){
        printf("The path to the data should be specified\n");
        return EXIT_SUCCESS;
    }
    
    string path_to_input_dir(argv[1]);
    
    printf("Path to data : %s\n", path_to_input_dir.c_str());
    
    // allocate and access files
    test_kvs_access(path_to_input_dir, "/home/paul/RUM/machines.csv");

    // send some files, create new kvs, recover the references
    //test_kvs_recover_by_hash(path_to_input_dir, servers);

    // remove remote files based on hash
    //test_kvs_remove_by_hash(path_to_input_dir, servers);
    
    return 0;
}

