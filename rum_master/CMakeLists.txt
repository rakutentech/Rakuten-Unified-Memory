
cmake_minimum_required (VERSION 3.1)
project (RUM C CXX)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 11)

set (CMAKE_CXX_FLAGS "-fPIC -fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")
set (CMAKE_C_FLAGS "-fPIC -fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")

set (DPDK_FILES
  nic.c
  packet.c
  dpdk_transition.cpp
)
set (FILES
  ${DPDK_FILES}
  rum.cpp
  rum_c_version.cpp
  address_lru.cpp
  Location.cpp
)

SET(DPDK_LIBS
	rte_net	     rte_ethdev   rte_distributor  rte_kni        rte_mbuf       	   rte_pmd_bond         rte_pmd_ring  	rte_power  
	rte_acl      rte_eal      rte_kvargs   	   rte_mempool    rte_mempool_ring	 rte_pmd_e1000        rte_pmd_virtio  rte_ring
	rte_cfgfile  rte_hash     rte_lpm      	   rte_meter      rte_pmd_i40e   	   rte_pmd_vmxnet3_uio  rte_sched       rte_timer
	rte_cmdline  rte_ip_frag  rte_pipeline 	   rte_pmd_ixgbe  rte_mempool_stack  rte_port             rte_table       rte_pmd_bnxt
	rte_pmd_ena  rte_pmd_nfp  rte_pdump
)

SET(ALL_LIBS
	-Wl,--whole-archive
	-Wl,--start-group ${DPDK_LIBS} -Wl,--end-group
	m pthread dl rt
	HEARTBEAT
	-Wl,--no-whole-archive
	numa	
)

include_directories (
  	${CMAKE_SOURCE_DIR}
	#libsp/src
	#libutil/src
	$ENV{RUM_HOME}/heartbeat
	$ENV{RUM_HOME}/paxos
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
	$ENV{RTE_SDK}/$ENV{RTE_TARGET}/lib
	$ENV{RUM_HOME}/lib
)

add_library (RUM STATIC ${FILES})
target_link_libraries (RUM ${ALL_LIBS})

