cmake_minimum_required (VERSION 3.1)
project (slave C CXX)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 11)

set (CMAKE_CXX_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")
set (CMAKE_C_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")

set (DPDK_FILES
  nic.c
  $ENV{RUM_HOME}/rum_master/packet.c
  SlavePaxos.cpp
  slave_paxos_msg.cpp
)
set (FILES
  ${DPDK_FILES}
  hpc_slave.cpp
)

SET(DPDK_LIBS
	rte_net	     rte_ethdev   rte_distributor  rte_kni        rte_mbuf       	   rte_pmd_bond         rte_pmd_ring  	rte_power  
	rte_acl      rte_eal      rte_kvargs   	   rte_mempool    rte_mempool_ring	 rte_pmd_e1000        rte_pmd_virtio  rte_ring
	rte_cfgfile  rte_hash     rte_lpm      	   rte_meter      rte_pmd_i40e   	   rte_pmd_vmxnet3_uio  rte_sched       rte_timer
	rte_cmdline  rte_ip_frag  rte_pipeline 	   rte_pmd_ixgbe  rte_mempool_stack  rte_port             rte_table       rte_pmd_bnxt
	rte_pmd_ena  rte_pmd_nfp  rte_pdump		   rte_pci		  rte_bus_pci		rte_bus_vdev
)

SET(ALL_LIBS
	-Wl,--whole-archive
	-Wl,--start-group ${DPDK_LIBS} -Wl,--end-group
	m pthread dl rt
	-Wl,--no-whole-archive
	numa
	boost_filesystem 
	boost_system 
    PAXOS
	HEARTBEAT
)

include_directories (
  	${CMAKE_SOURCE_DIR}
	$ENV{RUM_HOME}/rum_master
	$ENV{RUM_HOME}/paxos
	$ENV{RUM_HOME}/heartbeat
	$ENV{RTE_SDK}/$ENV{RTE_TARGET}/include
	$ENV{RTE_SDK}/drivers/net/bnxt
	$ENV{RTE_SDK}/drivers/net/ixgbe/base
	$ENV{RTE_SDK}/drivers/net/ixgbe
	$ENV{RTE_SDK}/drivers/net/i40e/base
	$ENV{RTE_SDK}/drivers/net/i40e
	$ENV{RTE_SDK}/drivers/net/e1000
	$ENV{RTE_SDK}/drivers/net/e1000/base
)

link_directories (
	$ENV{RUM_HOME}/lib
	$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib
)

ADD_EXECUTABLE(slave ${FILES})
TARGET_LINK_LIBRARIES(slave ${ALL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
