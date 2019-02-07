# RUM: Rakuten Unified Memory

**RUM** is an in-memory data grid library which uses user-space networking to achieve performance. Applications can use RUM to access memory located on servers. Several example applications have been included, such as a key value store, file system, and interpreter.


## How it works
* First, the slave nodes need to be started (`./build_slave.sh` && `./run_slave.sh`)
* All memory allocations need to be made via the rum memory allocation functions (`rum.h`)
    * Any application needs to call `rum_init()` before any allocations are made (this function will initialise the local settings)
    * All memory locations which are allocated both locally and remotely on a slave node. 
        * The slave node is chosen either explicity by the user or automatically via consistent hashing by RUM.
        * Local memory is allocated in virtual memory and protected, causing segmentation faults on access. This is how RUM is able to capture memory accesses without interferring with an application.
* At the end of the application, RUM will flush the cached memory to the slave nodes. 

**TL:DR** 
* call `rum_init(/path/to/machines.csv)` at the beginning of your application
* prefix all memory allocations with "rum_". e,g, `rum_alloc`, `rum_free`, `rum_calloc`, `rum_realloc`.

## Getting started
The main requirement is DPDK. This has been tested with version stable 18.02.2. The script 'setup.sh' has the information on how to export the necessary environment variables, as well as compilation flags. This work built the dpdk from source. 

Languages used are C++, and python for some configuration. There are many scripts which have been used to help with the process of building the different librarys that make up RUM, as well as the different example applications. 

#### Slave
To compile the slave: RUM_HOME/scripts/build_slave.sh
To run the slave    : RUM_HOME/scripts/run_slave.sh

Running the slave requires sudo (because dpdk requires sudo)

#### Rum Library
RUM_HOME/scripts/build_rum_lib.sh

#### Client
To compile a client application, the include files are found in RUM_HOME/rum_master, and the libraries can be found in RUM_HOME/lib. You will also need to include a bunch of libraries for the DPDK aspect. The example applications below have example CMakeLists.txt that can be followed as templates.


## Main Ingredients of RUM
| Name | Description | Purpose | Location |
|------|------|------|------|
|`Heartbeat`|This is a simple little library which is responsible for facilitating heartbeats between the clients and the slaves. | It is used to ensure that non-hash-based allocations are cleared up by the slaves when a client goes silent. Hash-based allocations are permanent (rum.h) |
|`Paxos`|This is an implementation of the PAXOS consensum algorithm.| It is used as mechanism to provide replication of the data across different RUM instances. The number and choice of backups can be configured.|
|`RUM`|This is the main RUM library that your application should be compiled against.|This brings together the above libraries as well as the RUM code itself.|

### Applications
A number of example aplications are included to show the different potential ways to use RUM:

| Name | Description | Purpose | Location |
|------|------|------|------|
|Memory Test| A simple application to exercise the different functions of RUM.|To test correctness.|RUM_HOME/apps/mem_test|
|Hashtable|A simple hashtable using RUM.|To show the base case of RUM integration.|RUM_HOME/apps/hashtable|
|Key Value Store|A key value store implementation using RUM|This is one of the more common usecases for distributed in-memory grids.|RUM_HOME/apps/kvs|
|S3CMD interface|A simple (and functional but incomplete) filesystem interface which uses the underlying KVS to create a filesysem.|To see how an KVS can be used to create an efficient filesystem.|RUM_HOME/apps/s3cmd|
|Lua Interpreter|Modified Version of the Lua interpreter to use RUM|To show that RUM is correct and can extend interpreted platforms to enable more memory access.|RUM_HOME/apps/lua|

## Limitations
1. As networking (via dpdk) is being used at the mac layer, there is no provision for reliable packet delivery. i.e. packets can be lost.
    1. This of course can be fixed, but was not the focus at the time of development.
2. `root` access is required to be able to replace the driver of a nic hardware port to be able to use dpdk.
3. Only ethernet has currently been tested, but networking support is given by dpdk so what is supports, this code supports.
4. Paxos works, however it is slow.

## Contributing
See the [contributing guide](CONTRIBUTING.md) for details of how to participate in development of the module.

## Changelog
See the [changelog](CHANGELOG.md) for the new features, changes and bug fixes of the module versions.
