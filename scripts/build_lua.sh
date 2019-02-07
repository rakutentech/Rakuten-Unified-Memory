#!/bin/sh

cd $RUM_HOME/app/lua
make clean
make linux 
make linux local
cd -