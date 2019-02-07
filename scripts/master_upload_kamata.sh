#!/bin/sh

$RUM_HOME/scripts/clean.sh

rsync -r ./rum_master 	paul@172.22.0.32:/home/paul/RUM/
rsync -r ./rum_slave  	paul@172.22.0.32:/home/paul/RUM/
rsync -r ./scripts	   	paul@172.22.0.32:/home/paul/RUM/
rsync -r ./paxos		paul@172.22.0.32:/home/paul/RUM/
rsync -r ./app	   	 	paul@172.22.0.32:/home/paul/RUM/
rsync -r ./heartbeat   	paul@172.22.0.32:/home/paul/RUM/
rsync -r ./machines.csv	paul@172.22.0.32:/home/paul/RUM/
