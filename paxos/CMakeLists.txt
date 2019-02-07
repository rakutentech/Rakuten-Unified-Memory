cmake_minimum_required (VERSION 3.1)
project (PAXOS C CXX)

set (CMAKE_VERBOSE_MAKEFILE ON)
set (CMAKE_CXX_STANDARD 11)

set (CMAKE_CXX_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")
set (CMAKE_C_FLAGS "-fno-stack-protector -Wall -Wextra -Wno-unused-parameter -g -O3 -march=native -msse4.2")

set (DPDK_FILES
    paxos_id.cpp
    paxos_timer.cpp
    paxos_instance.cpp
    paxos_msg.cpp
    UDP_comms.cpp
    TCP_comms.cpp
    util.h
    node.cpp
    Value.cpp
)
set (FILES
  ${DPDK_FILES}
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
	-Wl,--no-whole-archive
	 crypto 
)

include_directories (
  	${CMAKE_SOURCE_DIR}
)

link_directories (

)

add_library (PAXOS STATIC ${FILES})
target_link_libraries (PAXOS ${ALL_LIBS})

