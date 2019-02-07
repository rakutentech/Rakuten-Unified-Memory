#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "s3cmd_rum.h"

using namespace boost::program_options;
using namespace std;

string bucket_name;
string remote_path;
string local_path;
vector<string> files;
string string_to_sign;

void load_commands(options_description& cmds){
	cmds.add_options()
  		("mb", value<string>(&bucket_name), "Make bucket: mb /path/to/bucket")
  		("rb", value<string>(&bucket_name), "Remove bucket : rb s3://BUCKET")
  		("ls", value<string>(&bucket_name)->implicit_value("./"), "List objects or buckets :  [s3://BUCKET[/PREFIX]]")
  		("la", "List all object in all buckets")
  		("put", value<vector<string>>(&files)->multitoken(), "Put file into bucket: put FILE [FILE...] s3://BUCKET[/PREFIX]")
  		("get", value<vector<string>>(&files)->multitoken(), "Get file from bucket: get s3://BUCKET/OBJECT LOCAL_FILE")
  		("del", value<string>(&bucket_name), "Delete file from bucket:  del s3://BUCKET/OBJECT")
  		("sync", value<vector<string>>(&files)->multitoken(), "Synchronize a directory tree to S3: LOCAL_DIR s3://BUCKET[/PREFIX] or s3://BUCKET[/PREFIX] LOCAL_DIR")
  		("du", value<string>(&bucket_name)->implicit_value(""), "Disk usage by buckets: du [s3://BUCKET[/PREFIX]]")
  		("info", value<string>(&bucket_name)->implicit_value(""), "Get various information about Buckets or Files: info s3://BUCKET[/OBJECT]")
  		("cp", value<vector<string>>(&files)->multitoken(), "Copy object: cp s3://BUCKET1/OBJECT1 s3://BUCKET2[/OBJECT2]")
  		("mv", value<vector<string>>(&files)->multitoken(), "Move object: mv  s3://BUCKET1/OBJECT1 s3://BUCKET2[/OBJECT2]")
  		("setacl", value<string>(&bucket_name), "Modify Access control list for Bucket or Files: setacl s3://BUCKET[/OBJECT]")
  		("accesslog", value<string>(&bucket_name), "Enable/disable bucket access logging: accesslog s3://BUCKET")
  		("sign", value<string>(&string_to_sign), "Sign arbitrary string using the secret key: sign STRING-TO-SIGN")
  		("fixbucket", value<string>(&bucket_name), "Fix invalid file names in a bucket: fixbucket s3://BUCKET[/PREFIX]")
      ("jd", "a version of la which dumps in a json format")
  		;
}

void load_options(options_description& cmd_options){
	cmd_options.add_options()
      	("help,h", "show this help message and exit")
	    ("configure", "Invoke interactive (re)configuration tool.")
	    ("config=FILE,c", "Config file name. Defaults to /home/<username>/.s3cfg")
	    ("dump-config", "Dump current configuration after parsing config files and command line options and exit.")
	    ("dry-run,n", "Only show what should be uploaded or downloaded but don't actually do it. May still perform S3 requests to get bucket listings and other information though (only for file transfer commands)")
	    ("encrypt,e", "Encrypt files before uploading to S3.")
	    ("no-encrpyt", "Don't encrypt the files")
	    ("force,f", "Force overwrite and other dangerous operations.")
	    ("continue", "Continue getting a partially downloaded file (only for [get] command).")
	    ("skip-existing", "Skip over files that exist at the destination (only for [get] and [sync] commands).")
	    ("recursive,r", "Recursive upload, download, or removal")
		("check-md5", "Check MD5 sums when comparing files for [sync]. (default)")
		("no-check-md5", "Do not check MD5 sums when comparing files for [sync]. Only size will be compared. May significantly speed up transfer but may also miss some changed files.")
		("acl-public,P", "Store objects with ACL allowing read for anyone")
		("acl-private", "Store objects with default ACL allowing access for you only.")
		("acl-grant=PERMISSION:EMAIL or USER_CANONICAL_ID", "Grant stated permission to a given amazon user. Permission is one of: read, write, read_acp, write_acp, full_control, all")
		("acl-revoke=PERMISSION:USER_CANONICAL_ID", "Revoke stated permission for a given amazon user. Permission is one of: read, write, read_acp, wr ite_acp, full_control, all")
		("delete-removed", "Delete remote objects with no corresponding local file [sync]")
		("no-delete-removed", "Don't delete remote objects")
		("preserve,p", "Preserve filesystem attributes (mode, ownership, timestamps). Default for [sync] command.")
		("no-preserve", "Don't store FS attributes")
		("exclude=GLOB", "Filenames and paths matching GLOB will be excluded from sync")
		("include=GLOB", "Filenames and paths matching GLOB will be included even if previously excluded by one of --(r)exclude(-from) patterns")
		("include-from=FILE", "Read --include GLOBs from FILE")
		("rinclude=REGEXP", "Same as --include but uses REGEXP (regular expression) instead of GLOB")
		("rinclude-from=FILE", "Read --rinclude REGEXPs from FILE")
		("bucket-location=BUCKET_LOCATION", "Datacentre to create bucket in. As of now the datacenters are: US (default), EU, us-west-1, and ap- southeast-1")
		("reduced-redundancy,rr", "Store object with 'Reduced redundancy'. Lower per-GB price. [put, cp, mv]")
		("access-logging-target-prefix=LOG_TARGET_PREFIX", "Target prefix for access logs (S3 URI) (for [cfmodify] and [accesslog] commands)")
		("no-access-logging", "Disable access logging (for [cfmodify] and [accesslog] commands)")
		("mime-type=MIME/TYPE,m MIME/TYPE",  "Default MIME-type to be set for objects stored.")
		("guess-mime-type, M", "Guess MIME-type of files by their extension. Falls back to default MIME-Type as specified by --mime-type option")
		("add-header=NAME:VALUE", "Add a given HTTP header to the upload request. Can be used multiple times. For instance set 'Expires' or 'Cache-Control' headers (or both) using this options if you like.")
		("encoding=ENCODING", "Override autodetected terminal and filesystem encoding (character set). Autodetected: UTF-8")
		("verbatim", "Use the S3 name as given on the command line. No pre- processing, encoding, etc. Use with caution!")
		("list-md5", "Include MD5 sums in bucket listings (only for 'ls' command).")
		("human-readable-sizes,H", "Print sizes in human readable form (eg 1kB instead of 1234).")
		("progress", "Display progress meter (default on TTY).")
		("no-progress" , "Don't display progress meter (default on non-TTY).")
		("enable", "Enable given CloudFront distribution (only for [cfmodify] command)")
		("disable", "Enable given CloudFront distribution (only for [cfmodify] command)")
		("cf-add-cname=CNAME","Add given CNAME to a CloudFront distribution (only for [cfcreate] and [cfmodify] commands)")
		("cf-remove-cname=CNAME", "Remove given CNAME from a CloudFront distribution (only for [cfmodify] command)")
		("cf-comment=COMMENT", "Set COMMENT for a given CloudFront distribution (only for [cfcreate] and [cfmodify] commands)")
		("cf-default-root-object=DEFAULT_ROOT_OBJECT", "Set the default root object to return when no object is specified in the URL. Use a relative path, i.e. default/index.html instead of /default/index.html or s3://bucket/default/index.html (only for [cfcreate] and [cfmodify] commands)")
		("verbose,v", "Enable verbose output")
		("debug,d", "Enable debug output")
		("version", "Show s3cmd version (1.0.0) and exit.")
		("follow-symlinks,F", "Follow symbolic links as if they are regular files")
		;

}

int main(int argc, const char *argv[])
{
  try
  {
  	options_description cmds{"Commands"};
  	load_commands(cmds);

    options_description cmd_options{"Options"};
    load_options(cmd_options);

    variables_map vm;
    store(parse_command_line(argc, argv, cmds), vm);
    notify(vm);

    if (vm.count("help") || argc == 1){
		cout << cmds << '\n';
    }
    else if (vm.count("mb")){
      	cout << "mb: " << vm["mb"].as<string>() << '\n';
      	mb(vm["mb"].as<string>());
    }
    else if (vm.count("rb")){
      	cout << "rb: " << vm["rb"].as<string>() << '\n';
        rb(vm["rb"].as<string>().c_str());   
    }
    else if (vm.count("ls")){
      	cout << "ls:" << vm["ls"].as<string>()<< '\n';
        ls(vm["ls"].as<string>().c_str()); 
    }
    else if (vm.count("la")){
        cout << "la" << '\n';
        la(); 
    }
    else if (vm.count("put")){
      	cout << "put: " << '\n';
      	for(auto path : vm["put"].as<vector<string>>()){
      		printf("\t%s\n", path.c_str());
      	}

        std::vector<std::string> files = vm["put"].as<vector<string>>();
        put(std::vector<std::string>(
                                      files.begin(), 
                                      files.begin() + (files.size() - 1)
                                      ),
            files.at(files.size() - 1)
        );
    }
    else if (vm.count("get")){
      	cout << "get: " << '\n';
      	for(auto path : vm["get"].as<vector<string>>()){
      		printf("\t%s\n", path.c_str());
      	}
        get(vm["get"].as<vector<string>>().at(0), vm["get"].as<vector<string>>().at(1));
    }
    else if (vm.count("del")){
      	cout << "del: " << vm["del"].as<string>() << '\n';
        del(vm["del"].as<string>());
    }
    else if (vm.count("sync")){
      	cout << "sync: " << '\n';
      	for(auto path : vm["sync"].as<vector<string>>()){
      		printf("\t%s\n", path.c_str());
      	}
        sync(vm["sync"].as<vector<string>>().at(0), vm["sync"].as<vector<string>>().at(1));
    }
    else if (vm.count("du")){
        cout << "du " << vm["du"].as<string>() << '\n';
        du(vm["du"].as<string>());
    }
    else if (vm.count("info")){
        cout << "info " << vm["info"].as<string>() << '\n';
        info(vm["info"].as<string>());
    }
    else if (vm.count("cp")){
      	cout << "cp " << vm["cp"].as<vector<string>>().at(0) << vm["cp"].as<vector<string>>().at(1) << '\n';
        cp(vm["cp"].as<vector<string>>().at(0), vm["cp"].as<vector<string>>().at(1));
    }
    else if (vm.count("mv")){
      	cout << "mv " << vm["mv"].as<vector<string>>().at(0) << vm["mv"].as<vector<string>>().at(1) << '\n';
        mv(vm["mv"].as<vector<string>>().at(0), vm["mv"].as<vector<string>>().at(1));
    }
    else if (vm.count("setacl")){
      	cout << "setacl " << vm["setacl"].as<string>()<< '\n';
    }
    else if (vm.count("accesslog")){
      	cout << "accesslog " << vm["accesslog"].as<string>()<< '\n';
    }
    else if (vm.count("sign")){
      	cout << "sign " << vm["sign"].as<string>()<< '\n';
    }
    else if (vm.count("fixbucket")){
    	cout << "fixbucket " << vm["fixbucket"].as<string>()<< '\n';
    }
    else if(vm.count("jd")){
     cout << "jd " << '\n'; 
     jdump();
    }
    else{
    	cout << "something else" << "\n";
    }
  }
  catch (const error &ex)
  {
    std::cerr << ex.what() << '\n';
  }
}