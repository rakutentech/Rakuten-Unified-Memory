#!/bin/sh

$RUM_HOME/scripts/build_heartbeat.sh
$RUM_HOME/scripts/build_paxos.sh

rm -rf $RUM_HOME/build_slave
mkdir -p $RUM_HOME/build_slave
cd $RUM_HOME/build_slave
cmake ../rum_slave
make
cd $RUM_HOME/
mkdir -p $RUM_HOME/DATA
echo "====================================="
echo "======= Built the RUM Slave ======="
echo "====================================="
