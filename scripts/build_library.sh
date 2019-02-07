#!/bin/sh
rm -rf $RUM_HOME/lib/libRUM.a
mkdir -p $RUM_HOME/lib
cd $RUM_HOME/lib
cmake $RUM_HOME/rum_master
make
rm -rf CMakeFiles CMakeCache.txt cmake_install.cmake Makefile
cd -
echo "====================================="
echo "======= Built the RUM library ======="
echo "====================================="
