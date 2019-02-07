#!/bin/sh
#----
rm -rf $RUM_HOME/app/build_hashtable
mkdir -p $RUM_HOME/app/build_hashtable
cd $RUM_HOME/app/build_hashtable
cmake ../hash_table
make
cd -
echo "====================================="
echo "======= Built the hashtable app ====="
echo "====================================="