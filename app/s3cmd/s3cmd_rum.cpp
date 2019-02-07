#include <openssl/md5.h>	// for the hashing
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <iostream>
#include <fstream>

#include "json.hpp"

#include "hash_type.h"

#include "KVS.h"

#include "util.h"
#include <stdlib.h>     /* srand, rand */

#define INITIAL_ACCOUNTING_DATA_SIZE	1024

#define BUCKETS_FILE 	"BUCKETS.txt"
#define FILES_FILE 		"FILES.txt"

#define EMPTY_JSON		"{ \"buckets\" : [] }"

#define USE_LOCAL_FILE_SYS	0

using json = nlohmann::json;
//------------------------------------------------------------------------------
const char *machines_path = "/home/paul/RUM/machines.csv";
//------------------------------------------------------------------------------
void dump(Value *v){
	PRINTF("[%ld bytes] %s\n", v->get_len(), (char*)v->get_buffer());
	/*
	for(int i = 0 ; i < v->get_len() ; i++){
		PRINTF("[%d] %c (%d)\n", i, *((char*)v->get_buffer() + i), *((char*)v->get_buffer() + i));
	}
	*/
}
//------------------------------------------------------------------------------
std::vector<std::string> get_path_elements(std::string path){
	std::vector<std::string> path_strs;
 	std::stringstream ss(path);
 	std::string tmp;

 	while(ss.good()){
 		getline(ss, tmp, '/');
 		if(tmp.size() == 0){
 			continue;
 		}
 		path_strs.push_back(tmp);
 	}

	for(auto s : path_strs){
		printf("'%s' ", s.c_str());
		
	}
	printf("\n");

	return path_strs;
}
//------------------------------------------------------------------------------
void make_directory(std::string tab, std::vector<std::string>::iterator start, std::vector<std::string>::iterator end){
	if(start == end){
		return;
	}
	else{
		printf("%s{\"FOLDER\": \"%s\"\n",tab.c_str(), (*start).c_str());
		make_directory(tab + "\t", start + 1, end);
		printf("%s}\n", tab.c_str());
	}
}
//------------------------------------------------------------------------------
Value* check_file_system_exists(KVS* kvs, bool create_fs){
	Value *ret_val;
	char  *buf;

	// get the master bucket file
	if((ret_val = kvs->get(BUCKETS_FILE)) == NULL){
		PRINTF("try and make filesystem\n");
		if(create_fs){
			// does not yet exist, create

			if((buf = (char*)malloc(INITIAL_ACCOUNTING_DATA_SIZE)) == NULL){
				ERROR("could not allocate memory\n");
				return NULL;	
			}

			// write some accounting data
			memset(buf, '\0', INITIAL_ACCOUNTING_DATA_SIZE);
			memcpy(buf, EMPTY_JSON, strlen(EMPTY_JSON));

			Value* v = new Value(buf, INITIAL_ACCOUNTING_DATA_SIZE);
			kvs->add(BUCKETS_FILE, v);

			if((ret_val = kvs->get(BUCKETS_FILE)) == NULL){
				ERROR("cannot init the filesystem\n");
			}
		}
		else{
			ERROR("could not find filesystem\n");
		}
	}
	
	return ret_val;
}
//------------------------------------------------------------------------------
std::string load_acc_data(KVS *&kvs){
	#if USE_LOCAL_FILE_SYS
	// get the raw string
	std::ifstream t(BUCKETS_FILE);
	std::string str((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
	t.close();
#else
	Value *remote_ptr;

	kvs = new KVS(machines_path);

	if((remote_ptr = check_file_system_exists(kvs, true)) == NULL){
		std::string tmp("");
		return tmp;
	}
	std::string str;
	str.assign(((char*) remote_ptr->get_buffer()), strlen(remote_ptr->get_buffer()));
	//PRINTF("[%ld bytes] %s\n", remote_ptr->get_len(), (char*)remote_ptr->get_buffer());
	//dump(remote_ptr);
#endif
	return str;
}
//------------------------------------------------------------------------------
void save_acc_data(json j, KVS *kvs){
	printf("%s\n", j.dump(2).c_str());
	// delete the file, and write the new structure
#if USE_LOCAL_FILE_SYS
	std::ofstream output;
	output.open(BUCKETS_FILE, std::ofstream::out | std::ofstream::trunc);  		
	output << j.dump(4);
	output.close();
#else
	/*
	strcpy(remote_ptr->get_buffer(), j.dump().c_str());
	memset(remote_ptr->get_buffer() + j.dump().size(), '\0', 1);
	*/
	PRINTF("[%ld bytes] %s\n", j.dump().size(), j.dump().c_str());

	
 	char *s2 = strdup(j.dump().c_str());
	Value* v = new Value(s2, strlen(s2) + 1);
	//dump(v);
	kvs->add(BUCKETS_FILE, v);	
	//free(s2);
	PRINTF("pre-delete\n");
	delete v;
#endif
}
//------------------------------------------------------------------------------
// make bucket
void mb(std::string bucket_name){
	KVS *kvs;
	std::string str = load_acc_data(kvs);
	// parse the string
	json j = json::parse(str);
	printf("%s\n", j.dump(2).c_str());

	// check and see if there are any existing buckets
	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;

	std::vector<std::string> toks = get_path_elements(bucket_name);
	if(toks.size() == 0){
		ERROR("No bucket specified\n");
		return;
	}
	else if(toks.size() > 1){
		ERROR("You specified a path, not a bucket name\n");
	}

	if((ob_it = j.find("buckets")) != j.end()){
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				continue;
			}

			if((*field_it).is_string() && 
				((*field_it).get<std::string>().compare(toks.at(0)) == 0)){
				printf("found a match\n");
				return;
			}
		}
		// there was no such bucket, create and store
		json new_bucket;
		new_bucket["bucket_name"] 	= toks.at(0);
		new_bucket["file_list"] 	= nullptr;
		ob_it.value().push_back(new_bucket);
	}
	save_acc_data(j, kvs);
}
//------------------------------------------------------------------------------
//remove bucket
void rb(std::string bucket_name){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	printf("%s\n", j.dump(2).c_str());


	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	std::vector<std::string> toks = get_path_elements(bucket_name);

	if(toks.size() == 0){
		ERROR("No bucket specified\n");
		return;
	}
	else if(toks.size() > 1){
		ERROR("You specified a path, not a bucket name\n");
	}

	if((ob_it = j.find("buckets")) != j.end()){

		// we can not delete json array elements....
		json new_array;

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).is_string() && 
				((*field_it).get<std::string>().compare(toks.at(0)) == 0)){
				printf("found a match : %s\n", bucket_name.c_str());

				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					// we found our bucket, go through the files
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
						TODO("Delete the files\n");
#if !USE_LOCAL_FILE_SYS
						kvs->remove((*file_it)["hash"].get<std::string>());
#endif
					}
				}

				continue;
			}
			else{
				printf("no match\n");
			}

			new_array.push_back((*bkts_it));
		}

		// now we update the values and write
		j["buckets"] = new_array;
		printf("%s\n", j.dump(2).c_str());

		save_acc_data(j, kvs);
	}
}
//------------------------------------------------------------------------------
//list objects of buckets
void ls(std::string path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).is_string() && 
				((*field_it).get<std::string>().compare(path) == 0)){
				printf("%s: \n", path.c_str());

				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					// we found our bucket, go through the files
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
					}
				}

				// done 
				return;
			}
		}
	}
}
//------------------------------------------------------------------------------
// list all objects in all buckets
void la(){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				printf("%s: %ld entries\n", (*field_it).get<std::string>().c_str(), (*file_list_it).size());

				for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
					printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
				}
			}
			else{
				printf("%s: %d entries\n", (*field_it).get<std::string>().c_str(), 0);
			}
		}
	}
}
//------------------------------------------------------------------------------
// put file into bucket
void put(std::vector<std::string> paths, std::string remote_file_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);
	// parse the string
	json j = json::parse(str);

	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;
	json::iterator file_name_it;

	PRINTF("Copy %ld files to %s\n", paths.size(), remote_file_path.c_str());

	std::vector<std::string> remote_toks = get_path_elements(remote_file_path);

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			// does the name of this bucket match
			if((*field_it).is_string() && 
				((*field_it).get<std::string>().compare(remote_toks.at(0)) == 0)){
				printf("%s: \n", remote_toks.at(0).c_str());

				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					// we found our bucket, go through the files
					

					printf("%s\n", (*file_list_it).dump(2).c_str());
					// now for each file, we insert
					for(auto s : paths){

						bool found = false;
						for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
							if((*file_it)["file_name"].get<std::string>().compare(s) == 0){
								found = true;
								break;
							}
						}

						if(!found){
							// first we store the file
							fstream src_file;
							src_file.open(s, ios::in);
					        if(!src_file.is_open()){
					            printf("file %s not open\n", s.c_str());
					            return;
					        }
					        // get the file size
					        src_file.seekg(0, ios::end);
					        int len = src_file.tellg();
					        src_file.seekg(0, ios::beg);
					        
					        char* buf         = (char*)malloc(len);
					        
					        printf("Read %ld bytes from %s\n", len, s.c_str());
					        // get the data
					        src_file.read(buf, len);

					        // make the hash key
					        std::vector<std::string> f_toks = get_path_elements(s);
					        std::string new_hash;
				        	new_hash += remote_toks.at(0);
				        	new_hash += "/";
				        	new_hash += f_toks.at(f_toks.size() - 1);

#if !USE_LOCAL_FILE_SYS
					        Value* v = new Value(buf, len);
				        	kvs->add(new_hash, v);
#endif				        	
				        	src_file.close();
				        	free(buf);

				        	// now do accounting
				        	
				        	
							json new_bucket;
							new_bucket["file_name"] = f_toks.at(f_toks.size() - 1);
							new_bucket["hash"] = new_hash;
							new_bucket["size"] = len;
							TODO("Things like last modified, etc...\n");
							/*
							* eof/len
							* access control
							* symlinks??
							* modification time
							* creation time
							*/
							//ob_it.value().push_back(bucket_name.c_str());
							file_list_it.value().push_back(new_bucket);
						}
						else{
							printf("'%s' already exists in bucket '%s'\n", s.c_str(), remote_file_path.c_str());
						}
					}
					
					printf("%s\n", j.dump(2).c_str());
					save_acc_data(j, kvs);

					// added our file, so done
					return;
				}
			}
		}
		// we only get here if we did not succeed
		ERROR("Could not find bucket '%s'\n", remote_toks.at(0).c_str());
	}
}
//------------------------------------------------------------------------------
// get file from bucket
void get(std::string remoteObj_path, std::string local_file_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);

	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;
	json::iterator file_name_it;

	bool found_bucket 	= false;
	bool found_file		= false;

	printf("Copy from '%s' to '%s'\n", remoteObj_path.c_str(), local_file_path.c_str());

	std::vector<std::string> remote_toks = get_path_elements(remoteObj_path);
	if(remote_toks.size() != 2){
		ERROR("invalid path to object\n");
		return;
	}

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			// does the name of this bucket match
			if((*field_it).is_string() && 
				((*field_it).get<std::string>().compare(remote_toks.at(0)) == 0)){
				printf("%s: \n", remote_toks.at(0).c_str());
				found_bucket = true;

				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					// we found our bucket, go through the files
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());

						if((*file_it)["file_name"].get<std::string>().compare(remote_toks.at(1)) == 0){
							TODO("Found object '%s', copy to '%s'\n", remote_toks.at(1).c_str(), local_file_path.c_str());
							found_file = true;
#if !USE_LOCAL_FILE_SYS
							Value* v = kvs->get((*file_it)["hash"].get<std::string>());

#endif

							ofstream out_file(local_file_path);
					        if(!out_file){
					            printf("file %s not open\n", local_file_path.c_str());
					            return;
					        }
					        // get the file size
					       	out_file.write(v->get_buffer(), v->get_len());
					       	out_file.close();
					       	return;
						}
					}
					// we only get here if we did not succeed
					if(!found_file){
						ERROR("Could not find remote file '%s'\n", remote_toks.at(1).c_str());	
					}
				}
			}
		}
		// we only get here if we did not succeed
		if(!found_bucket){
			ERROR("Could not find bucket '%s'\n", remote_toks.at(0).c_str());	
		}
	}
}
//------------------------------------------------------------------------------
// delete file from bucket
void del(std::string remote_object_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	PRINTF("%s\n", j.dump(2).c_str());

	bool found_bucket = false;;
	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;
	json::iterator file_name_it;

	printf("delete '%s' \n", remote_object_path.c_str());

	std::vector<std::string> remote_toks = get_path_elements(remote_object_path);

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			// does the name of this bucket match
			if((*field_it).is_string() && 
				((*field_it).get<std::string>().compare(remote_toks.at(0)) == 0)){
				printf("%s: \n", remote_toks.at(0).c_str());

				found_bucket = true;
				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					// we can not delete json array elements....
					json new_array;

					// we found our bucket, go through the files
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());

						if((*file_it)["file_name"].get<std::string>().compare(remote_toks.at(1)) == 0){
							printf("found file to delete\n");
#if !USE_LOCAL_FILE_SYS
							kvs->remove((*file_it)["hash"].get<std::string>());
#endif
							continue;
						}

						new_array.push_back(*file_it);
					}

					// now we update the values and write
					(*bkts_it)["file_list"] = new_array;
					save_acc_data(j, kvs);

					return;
				}
			}
		}
		if(!found_bucket){
			// we only get here if we did not succeed
			ERROR("Could not find bucket '%s'\n", remote_toks.at(0).c_str());
		}
	}
}
//------------------------------------------------------------------------------
// Synchronise a directory tree to S3
void sync_remote_to_local(char* remote_bucket_path, char* local_dir_path){

}
//------------------------------------------------------------------------------
// Synchronise a directory tree to S3
void sync_local_to_remote(char* local_dir_path, char* remote_bucket_path){

}
//------------------------------------------------------------------------------
void sync(std::string a, std::string b){
	TODO("sync\n");
}
//------------------------------------------------------------------------------
/*
 * Disk usage by buckets
 *  
 * There are 3 cases: no path, bucket, file 
 */
void du(std::string remote_bucket_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	size_t total = 0;

	std::vector<std::string> toks = get_path_elements(remote_bucket_path);

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if(toks.size() > 0 && (*field_it).get<std::string>().compare(toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}

			if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				printf("%s: %ld entries\n", (*field_it).get<std::string>().c_str(), (*file_list_it).size());

				for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
					printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());

					if(toks.size() > 1 && (*file_it)["file_name"].get<std::string>().compare(toks.at(1)) != 0){
						// we are looking for a specific file, and this aint it
						continue;
					}
					total += (*file_it)["size"].get<int>();
				}
			}
		}
	}

	if(toks.size() > 0){
		printf("Total of '%s' is %ld bytes\n", remote_bucket_path.c_str(), total);
	}
	else{
		printf("Total is %ld bytes\n", total);	
	}
	
}
//------------------------------------------------------------------------------
// get various information about buckets of files
void info(std::string remote_bucket_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	std::vector<std::string> toks = get_path_elements(remote_bucket_path);

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if(toks.size() > 0 && (*field_it).get<std::string>().compare(toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}


			if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				printf("%s: %ld entries\n", (*field_it).get<std::string>().c_str(), (*file_list_it).size());

				for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
					

					if(toks.size() == 2 && (*file_it)["file_name"].get<std::string>().compare(toks.at(1)) == 0){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
						TODO("Dump some info on files ...\n");
					}
					
				}
			}

			if(toks.size() == 1){
				TODO("Dump some info on bucket ...\n");
			}
		}
	}
}
//------------------------------------------------------------------------------
// copy object
void cp(std::string remote_object1_path, std::string remote_object_or_bucket_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);
	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	bool found_bucket;
	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	std::vector<std::string> from_toks = get_path_elements(remote_object1_path);
	std::vector<std::string> to_toks   = get_path_elements(remote_object_or_bucket_path);

	if(to_toks.size() == 1){
		printf("copy to bucket\n");
	}
	else if(to_toks.size() == 2){
		printf("copy to file\n");
	}

	json from_file;

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		found_bucket = false;
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).get<std::string>().compare(from_toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}
			found_bucket = true;

			if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				printf("%s: %ld entries\n", (*field_it).get<std::string>().c_str(), (*file_list_it).size());

				bool found = false;
				for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
					if((*file_it)["file_name"].get<std::string>().compare(from_toks.at(1)) == 0){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
						from_file = (*file_it);
						found = true;
						break;
					}
				}
				if(!found){
					ERROR("Could not find file '%s'\n", from_toks.at(1).c_str());
					return;
				}
			}
		}

		if(!found_bucket){
			ERROR("Bucket '%s' does not exist\n", from_toks.at(0).c_str());
			return;
		}

		// now find the place to copy to
		// for each bucket object
		found_bucket = false;
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).get<std::string>().compare(to_toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}

			found_bucket = true;
			if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				if(to_toks.size() == 2){
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						if((*file_it)["file_name"].get<std::string>().compare(to_toks.at(1)) == 0){
							TODO("replace a file\n");
							break;
						}
					}
				}
				else if (to_toks.size() == 1){
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						if((*file_it)["file_name"].get<std::string>().compare(from_toks.at(1)) == 0){
							ERROR("file '%s' already exists in bucket '%s'\n", 
								from_toks.at(1).c_str(), to_toks.at(0).c_str());
							return;
						}
					}
					

					// now clone the file
					std::string new_hash;
					new_hash += to_toks.at(0);
					new_hash += "/";
					new_hash += from_file["file_name"];
#if !USE_LOCAL_FILE_SYS
					Value *v;
					if((v = kvs->get(from_file["hash"].get<std::string>())) == NULL){
						ERROR("could not access file '%s'\n", from_file["hash"].get<std::string>().c_str());
						return;
					}

					kvs->add(new_hash, v);
#endif		

					json new_bucket;
					new_bucket["file_name"] = from_file["file_name"];
					new_bucket["hash"] = new_hash;
					new_bucket["size"] = from_file["size"];
					// all good, move the file
					file_list_it.value().push_back(new_bucket);	
				}
			}
		}
		if(!found_bucket){
			ERROR("Bucket '%s' does not exist\n", to_toks.at(0).c_str());
			return;
		}
	}
	printf("%s\n", j.dump(2).c_str());
	save_acc_data(j, kvs);
}
//------------------------------------------------------------------------------
// move object
void mv(std::string remote_object1_path, std::string remote_object_or_bucket_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	bool found_bucket;
	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	std::vector<std::string> from_toks = get_path_elements(remote_object1_path);
	std::vector<std::string> to_toks = get_path_elements(remote_object_or_bucket_path);

	if(from_toks.size() == 0 || to_toks.size() == 0){
		ERROR("no bucket or file specified\n");
		return;
	}
	else if(from_toks.size() > 2 || to_toks.size() > 2){
		ERROR("Path too long\n");
		return;	
	}
	else if(to_toks.size() == 1){
		printf("move to bucket\n");
	}
	else if(to_toks.size() == 2){
		printf("move to file\n");
	}

	json from_file;

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		found_bucket = false;
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).get<std::string>().compare(from_toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}
			found_bucket = true;

			if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				printf("%s: %ld entries\n", (*field_it).get<std::string>().c_str(), (*file_list_it).size());
				
				json new_array;
				bool found = false;
				for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
					if((*file_it)["file_name"].get<std::string>().compare(from_toks.at(1)) == 0){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
						from_file = (*file_it);
						found = true;
						continue;
					}
					new_array.push_back(*file_it);
				}
				if(!found){
					ERROR("Could not find file '%s'\n", from_toks.at(1).c_str());
					return;
				}

				PRINTF("new file list %s\n", new_array.dump(2).c_str());
				(*bkts_it)["file_list"] = new_array;
				//PRINTF("new file list %s\n", j.dump(2).c_str());
			}
		}

		if(!found_bucket){
			ERROR("Bucket '%s' does not exist\n", from_toks.at(0).c_str());
			return;
		}

		PRINTF("File to move %s\n", from_file.dump(2).c_str());

		// refresh the json object with the above modification
		//j = json::parse(j.dump(2));
		//ob_it = j.find("buckets");

		PRINTF("new file list %s\n", j.dump(2).c_str());

		// now find the place to copy to
		// for each bucket object
		found_bucket = false;
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).get<std::string>().compare(to_toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}

			found_bucket = true;
			if(to_toks.size() == 2){
				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						if((*file_it)["file_name"].get<std::string>().compare(to_toks.at(1)) == 0){
							TODO("replace a file\n");
							break;
						}
					}
				}
			}
			else if (to_toks.size() == 1){
				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());
						if((*file_it)["file_name"].get<std::string>().compare(from_toks.at(1)) == 0){
							ERROR("file '%s' already exists in bucket '%s'\n", 
								from_toks.at(1).c_str(), to_toks.at(0).c_str());
							return;
						}
					}
				}

				// now clone the file
				std::string new_hash;
				new_hash += to_toks.at(0);
				new_hash += "/";
				new_hash += from_file["file_name"];
#if !USE_LOCAL_FILE_SYS
				Value *v;
				if((v = kvs->get(from_file["hash"].get<std::string>())) == NULL){
					ERROR("could not access file '%s'\n", from_file["hash"].get<std::string>().c_str());
					return;
				}

				kvs->add(new_hash, v);
#endif		

				json new_bucket;
				new_bucket["file_name"] = from_file["file_name"];
				new_bucket["hash"] = new_hash;
				new_bucket["size"] = from_file["size"];
				// all good, move the file
				file_list_it.value().push_back(new_bucket);	
			}
		}
		if(!found_bucket){
			ERROR("Bucket '%s' does not exist\n", to_toks.at(0).c_str());
			return;
		}
	}
	save_acc_data(j, kvs);
}
//------------------------------------------------------------------------------
// modify access control list for bucket
void setacl(std::string remote_object_or_bucket_path){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	//printf("%s\n", j.dump(2).c_str());

	bool found_bucket;
	bool bucket_only = false;
	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;

	std::vector<std::string> toks = get_path_elements(remote_object_or_bucket_path);

	if(toks.size() == 0){
		ERROR("no bucket or file specified\n");
		return;
	}
	else if(toks.size() > 2){
		ERROR("Path too long\n");
		return;	
	}

	if(toks.size() == 2){
		bucket_only = false;
	}

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		found_bucket = false;
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			if((*field_it).get<std::string>().compare(toks.at(0)) != 0){
				// we are looking for a specific bucket, and this aint it
				continue;
			}
			found_bucket = true;

			if(bucket_only){
				TODO("BUCKET - SET ACCESS CONTROL\n");
			}

			if(!bucket_only && 
				(file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
				printf("%s: %ld entries\n", (*field_it).get<std::string>().c_str(), (*file_list_it).size());
				
				json new_array;
				bool found = false;
				for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
					if((*file_it)["file_name"].get<std::string>().compare(toks.at(1)) == 0){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());

						TODO("FILE - SET ACCESS CONTROL\n");
						found = true;
						continue;
					}
					new_array.push_back(*file_it);
				}
				if(!found){
					ERROR("Could not find file '%s'\n", toks.at(1).c_str());
					return;
				}

				PRINTF("new file list %s\n", new_array.dump(2).c_str());
				(*bkts_it)["file_list"] = new_array;
				//PRINTF("new file list %s\n", j.dump(2).c_str());
			}
		}

		if(!found_bucket){
			ERROR("Bucket '%s' does not exist\n", toks.at(0).c_str());
			return;
		}
	}
	save_acc_data(j, kvs);
}
//------------------------------------------------------------------------------
// Enable/disable bucket access logging
void accesslog(std::string remote_bucket_path){

}
//------------------------------------------------------------------------------
// sign arbitrary string using the secret key
void sign(std::string string_to_follow){

}
//------------------------------------------------------------------------------
// Fix invalid names in bucket
void fixbucket(std::string remote_object_or_bucket_path){

}
//------------------------------------------------------------------------------
void jdump(){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	printf("%s\n", str.c_str());
	json j = json::parse(str);
	printf("%s\n", j.dump(2).c_str());
}
//------------------------------------------------------------------------------
void clear_all_buckets(){
	KVS *kvs;
	std::string str = load_acc_data(kvs);

	// parse the string
	json j = json::parse(str);
	PRINTF("%s\n", j.dump(2).c_str());

	bool found_bucket = false;;
	json::iterator ob_it;
	json::iterator bkts_it;
	json::iterator field_it;
	json::iterator file_list_it;
	json::iterator file_it;
	json::iterator file_name_it;

	printf("delete all \n");

	if((ob_it = j.find("buckets")) != j.end()){

		// for each bucket object
		for(bkts_it = ob_it.value().begin(); bkts_it != ob_it.value().end(); ++bkts_it){

			// find the buckets name field
			if((field_it = (*bkts_it).find("bucket_name")) == (*bkts_it).end()){
				// this should never happen...
				continue;
			}

			// does the name of this bucket match
			if((*field_it).is_string()){
				printf("%s: \n", (*field_it).get<std::string>().c_str());

				if((file_list_it = (*bkts_it).find("file_list")) != (*bkts_it).end()){
					// we can not delete json array elements....
					json new_array;

					// we found our bucket, go through the files
					for(file_it = file_list_it.value().begin() ; file_it != file_list_it.value().end() ; ++file_it){
						printf("\t%s\n", (*file_it)["file_name"].get<std::string>().c_str());

#if !USE_LOCAL_FILE_SYS
						kvs->remove((*file_it)["hash"].get<std::string>());
#endif
					}

					// now we update the values and write
					(*bkts_it)["file_list"] = new_array;
				}
			}
		}
	}
	j["buckets"].clear();
	save_acc_data(j, kvs);
	PRINTF("%s\n", j.dump(2).c_str());
}