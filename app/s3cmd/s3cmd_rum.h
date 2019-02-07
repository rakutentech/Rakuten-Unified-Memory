#ifndef __S3CMD_RUM_H__
#define __S3CMD_RUM_H__

#include <string>
#include <vector>

void mb(std::string bucket_name);
void rb(std::string bucket_name);
void ls(std::string path);
void la(void);
void put(std::vector<std::string> paths, std::string remote_file_path);
void get(std::string remoteObj_path, std::string local_file_path);
void del(std::string remote_object_path);
void sync(std::string a, std::string b);
void du(std::string remote_bucket_path);
void info(std::string remote_bucket_path);
void cp(std::string remote_object1_path, std::string remote_object_or_bucket_path);
void mv(std::string remote_object1_path, std::string remote_object_or_bucket_path);
void jdump();
void clear_all_buckets(void);

#endif /*__S3CMD_RUM_H__*/