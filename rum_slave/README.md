#The RUM Slave
The RUM slave is a daemon which should be run on nodes which participate in the RUM system (party :) ). The purpose is to listen for packets from some master, and to reply accordingly. 

The RUM slave is designed to work with the RUM master. Together, the RUM slave and the RUM master form the RUM system (party).
## Installation

### Requirements
The RUM slave requires the dpdk environment. At the moment, this is working with dpdk-stable-18.02.2.

## Help
Help in writing this document can be found here : https://www.makeareadme.com

## Requirements

* cmake
* C89
* C++11

## Build

How to build the HPC slave node.

```
./domake l2eve
```

## Run

```
./build/dpdk_main -- -m 28 -s 12 -p 1
./build/dpdk_main -- -m 28 -s 8 -p 1
```


Command line options:

```
static const char short_options[] =
	"p:"  /* portmask */
	"q:"  /* number of queues */
	"T:"  /* timer period */
	"s:"  /* page size in bits */
	"m:"  /* mem pool size in bits */
	;
```
