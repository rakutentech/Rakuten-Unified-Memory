#!/bin/sh

$RUM_HOME/clean.sh

scp -P 32022 -r ./rum_master 		paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./rum_slave  		paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./LD_PRELOAD  		paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./app		  	 	paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./paxos		  		paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./scripts	  		paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./heartbeat	  		paul@118.238.211.43:/home/paul/RUM/

