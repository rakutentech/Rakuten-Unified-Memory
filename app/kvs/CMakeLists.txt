CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(kvs CXX)

SET(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package (Boost 1.48.0 COMPONENTS program_options REQUIRED)

find_package (Threads)

SET(CMAKE_CXX_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")
SET(CMAKE_C_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")

SET(FILES
	KVS
	hashring
	Key
	Value
	main
)

SET(DPDK_LIBS
	rte_net	     rte_ethdev   rte_distributor  rte_kni        rte_mbuf       	           	rte_power  
	rte_acl      rte_eal      rte_kvargs   	   rte_mempool    rte_mempool_ring	           rte_ring
	rte_cfgfile  rte_hash     rte_lpm      	   rte_meter         	     rte_sched       rte_timer
	rte_cmdline  rte_ip_frag  rte_pipeline 	   rte_pmd_ixgbe  rte_mempool_stack  rte_port             rte_table    rte_pdump	rte_pci			 rte_bus_pci
)

SET(ALL_LIBS
	-Wl,--whole-archive
	-Wl,--start-group ${DPDK_LIBS} -Wl,--end-group
	m dl rt 
	-Wl,--no-whole-archive
	boost_filesystem 
	boost_system 
	crypto 
	pthread
	numa
	RUM
	HEARTBEAT
)

INCLUDE_DIRECTORIES(
	$ENV{RUM_HOME}/rum_master
	$ENV{RTE_SDK}/$ENV{RTE_TARGET}/include
	$ENV{RTE_SDK}/drivers/net/ixgbe/base
	$ENV{RTE_SDK}/drivers/net/ixgbe
)

LINK_DIRECTORIES(
	$ENV{RUM_HOME}/lib
	$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib
)

ADD_EXECUTABLE(kvs ${FILES})
TARGET_LINK_LIBRARIES(kvs ${ALL_LIBS} ${Boost_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
