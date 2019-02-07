#!/bin/sh
#----
rm -rf $RUM_HOME/app/build_tests
mkdir -p $RUM_HOME/app/build_tests
cd $RUM_HOME/app/build_tests
cmake ../tests
make
cd $RUM_HOME/
echo "====================================="
echo "======= Built the mem tests ========="
echo "====================================="