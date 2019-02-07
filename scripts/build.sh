#!/bin/sh
rm -rf $RUM_HOME/lib
mkdir -p $RUM_HOME/lib
cd $RUM_HOME/lib
cmake ../rum_master
make
cd $RUM_HOME
echo "====================================="
echo "======= Built the RUM library ======="
echo "====================================="
#----
rm -rf $RUM_HOME/app/build_kvs
mkdir -p $RUM_HOME/app/build_kvs
cd $RUM_HOME/app/build_kvs
cmake ../kvs
make
cd $RUM_HOME
echo "====================================="
echo "======= Built the KVS app ==========="
echo "====================================="
#----
rm -rf $RUM_HOME/app/build_hashtable
mkdir -p $RUM_HOME/app/build_hashtable
cd $RUM_HOME/app/build_hashtable
cmake ../hash_table
make
cd $RUM_HOME
echo "====================================="
echo "======= Built the hashtable app ====="
echo "====================================="