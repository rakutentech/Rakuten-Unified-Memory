#!/bin/sh

rsync -r ./rum_master paul@172.22.0.31:/home/paul/RUM
rsync -r ./rum_slave  paul@172.22.0.31:/home/paul/RUM/
rsync -r ./paxos  	paul@172.22.0.31:/home/paul/RUM/
rsync -r ./heartbeat	paul@172.22.0.31:/home/paul/RUM/
rsync -r ./scripts	paul@172.22.0.31:/home/paul/RUM/

#rsync -r ./rum_master paul@172.22.0.32:/home/paul/RUM/
#rsync -r ./rum_slave  paul@172.22.0.32:/home/paul/RUM/
#rsync -r ./build.sh   paul@172.22.0.32:/home/paul/RUM/
#rsync -r ./kvs   	 	paul@172.22.0.32:/home/paul/RUM/

rsync -r ./rum_master paul@172.22.0.33:/home/paul/RUM/
rsync -r ./rum_slave  paul@172.22.0.33:/home/paul/RUM/
rsync -r ./paxos 		paul@172.22.0.33:/home/paul/RUM/
rsync -r ./heartbeat	paul@172.22.0.33:/home/paul/RUM/
rsync -r ./scripts	paul@172.22.0.33:/home/paul/RUM/

rsync -r ./rum_master paul@172.22.0.34:/home/paul/RUM/
rsync -r ./rum_slave  paul@172.22.0.34:/home/paul/RUM/
rsync -r ./paxos 		paul@172.22.0.34:/home/paul/RUM/
rsync -r ./heartbeat	paul@172.22.0.34:/home/paul/RUM/
rsync -r ./scripts	paul@172.22.0.34:/home/paul/RUM/

rsync -r ./rum_master paul@172.22.0.35:/home/paul/RUM/
rsync -r ./rum_slave  paul@172.22.0.35:/home/paul/RUM/
rsync -r ./paxos 		paul@172.22.0.35:/home/paul/RUM/
rsync -r ./heartbeat	paul@172.22.0.35:/home/paul/RUM/
rsync -r ./scripts	paul@172.22.0.35:/home/paul/RUM/