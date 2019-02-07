#!/bin/sh

$RUM/clean.sh

PORT = 31022

scp -P $PORT -r ./rum_master 	paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./rum_slave  	paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./rum_master/packet.* paul@118.238.211.43:/home/paul/RUM/rum_master/
scp -P $PORT -r ./paxos  		paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./app   		paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./heartbeat   	paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./scripts   	paul@118.238.211.43:/home/paul/RUM/

PORT = 33022

scp -P $PORT -r ./rum_master paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./rum_slave  paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./rum_master/packet.* paul@118.238.211.43:/home/paul/RUM/rum_master/
scp -P $PORT -r ./paxos  		paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./app   paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./heartbeat   	paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./scripts		paul@118.238.211.43:/home/paul/RUM/

PORT = 34022

scp -P $PORT -r ./rum_master paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./rum_slave  paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./rum_master/packet.* paul@118.238.211.43:/home/paul/RUM/rum_master/
scp -P $PORT -r ./paxos  		paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./app   paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./heartbeat   paul@118.238.211.43:/home/paul/RUM/
scp -P $PORT -r ./scripts		paul@118.238.211.43:/home/paul/RUM/




