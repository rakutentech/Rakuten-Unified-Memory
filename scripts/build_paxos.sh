#!/bin/sh
rm -rf $RUM_HOME/lib/libPAXOS.a
mkdir -p $RUM_HOME/lib
cd $RUM_HOME/lib
cmake $RUM_HOME/paxos
make
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
cd $RUM_HOME/
echo "====================================="
echo "===== Built the PAXOS library ======="
echo "====================================="