#!/bin/sh

scp -P 31022 -r ./rum_master paul@118.238.211.43:/home/paul/RUM/
scp -P 31022 -r ./rum_slave  paul@118.238.211.43:/home/paul/RUM/
scp -P 31022 -r ./build.sh   paul@118.238.211.43:/home/paul/RUM/
scp -P 31022 -r ./paxos   	 paul@118.238.211.43:/home/paul/RUM/
scp -P 31022 -r ./app/kvs  	 paul@118.238.211.43:/home/paul/RUM/app/
scp -P 31022 -r ./app/hash_table  	 paul@118.238.211.43:/home/paul/RUM/app/

scp -P 32022 -r ./rum_master paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./rum_slave  paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./build.sh   paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./paxos		 paul@118.238.211.43:/home/paul/RUM/
scp -P 32022 -r ./app/kvs  	 paul@118.238.211.43:/home/paul/RUM/app/
scp -P 32022 -r ./app/hash_table  	 paul@118.238.211.43:/home/paul/RUM/app/

scp -P 33022 -r ./rum_master paul@118.238.211.43:/home/paul/RUM/
scp -P 33022 -r ./rum_slave  paul@118.238.211.43:/home/paul/RUM/
scp -P 33022 -r ./build.sh   paul@118.238.211.43:/home/paul/RUM/
scp -P 33022 -r ./paxos		 paul@118.238.211.43:/home/paul/RUM/
scp -P 33022 -r ./app/kvs  	 paul@118.238.211.43:/home/paul/RUM/app/
scp -P 33022 -r ./app/hash_table  	 paul@118.238.211.43:/home/paul/RUM/app/