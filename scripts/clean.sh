#!/bin/sh

rm -rf $RUM_HOME/app/build_*
rm -rf $RUM_HOME/build_slave
rm -rf $RUM_HOME/heartbeat/build
cd $RUM_HOME/app/lua
rm -rf ./bin ./lib ./liblua.a ./temp
make clean
cd -
cd $RUM_HOME/app/paxos_server
make clean
cd -
