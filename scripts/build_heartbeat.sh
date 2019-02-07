#!/bin/sh
rm -rf $RUM_HOME/lib/libHEARTBEAT.a
mkdir -p $RUM_HOME/lib
cd $RUM_HOME/lib
cmake $RUM_HOME/heartbeat
make
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
cd -
echo "====================================="
echo "=== Built the HEARTBEAT library ====="
echo "====================================="
