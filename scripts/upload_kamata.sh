#!/bin/sh

scp -r ./rum_master paul@172.22.0.31:/home/paul/RUM/
scp -r ./rum_slave  paul@172.22.0.31:/home/paul/RUM/
scp -r ./build.sh   paul@172.22.0.31:/home/paul/RUM/
scp -r ./app/kvs   	paul@172.22.0.31:/home/paul/RUM/app/
scp -r ./app/hash_table   	paul@172.22.0.31:/home/paul/RUM/app/

scp -r ./rum_master paul@172.22.0.32:/home/paul/RUM/
scp -r ./rum_slave  paul@172.22.0.32:/home/paul/RUM/
scp -r ./build.sh   paul@172.22.0.32:/home/paul/RUM/
scp -r ./app/kvs   	paul@172.22.0.32:/home/paul/RUM/app/
scp -r ./app/hash_table   	paul@172.22.0.32:/home/paul/RUM/app/

scp -r ./rum_master paul@172.22.0.33:/home/paul/RUM/
scp -r ./rum_slave  paul@172.22.0.33:/home/paul/RUM/
scp -r ./build.sh   paul@172.22.0.33:/home/paul/RUM/
scp -r ./app/kvs   	paul@172.22.0.33:/home/paul/RUM/app/
scp -r ./app/hash_table   	paul@172.22.0.33:/home/paul/RUM/app/