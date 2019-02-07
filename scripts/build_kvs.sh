#!/bin/sh
#----
rm -rf $RUM_HOME/app/build_kvs
mkdir -p $RUM_HOME/app/build_kvs
cd $RUM_HOME/app/build_kvs
cmake ../kvs
make
cd -
echo "====================================="
echo "======= Built the KVS app ==========="
echo "====================================="
