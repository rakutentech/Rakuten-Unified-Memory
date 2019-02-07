#!/bin/sh

$RUM_HOME/scripts/build_heartbeat.sh
$RUM_HOME/scripts/build_paxos.sh
$RUM_HOME/scripts/build_library.sh
$RUM_HOME/scripts/build_slave.sh

$RUM_HOME/scripts/build_kvs.sh
$RUM_HOME/scripts/build_hashtable.sh
$RUM_HOME/scripts/build_paxos_server.sh
$RUM_HOME/scripts/build_lua.sh
