#include "nic.h"

/* Check the link status of all ports in up to 9s, and print them finally */
// Reference: l2fwd example
void
dpdk_check_link_status(uint8_t port_num, uint32_t port_mask)
{
#define CHECK_INTERVAL 100 /* 100ms */
#define MAX_CHECK_TIME 90 /* 9s (90 * 100ms) in total */
	uint8_t portid, count, all_ports_up, print_flag = 0;
	struct rte_eth_link link;

	printf("\nChecking link status");
	fflush(stdout);
	for (count = 0; count <= MAX_CHECK_TIME; count++) {
		all_ports_up = 1;
		for (portid = 0; portid < port_num; portid++) {
			if ((port_mask & (1 << portid)) == 0)
				continue;
			memset(&link, 0, sizeof(link));
			rte_eth_link_get_nowait(portid, &link);
			/* print link status if flag set */
			if (print_flag == 1) {
				if (link.link_status)
					printf("Port %d Link Up - speed %u "
						"Mbps - %s\n", (uint8_t)portid,
						(unsigned)link.link_speed,
				(link.link_duplex == ETH_LINK_FULL_DUPLEX) ?
					("full-duplex") : ("half-duplex\n"));
				else
					printf("Port %d Link Down\n",
							(uint8_t)portid);
				continue;
			}
			/* clear all_ports_up flag if any link down */
			if (link.link_status == ETH_LINK_DOWN) {
				all_ports_up = 0;
				break;
			}
		}
		/* after finally printing all link status, get out */
		if (print_flag == 1)
			break;

		if (all_ports_up == 0) {
			printf(".");
			fflush(stdout);
			rte_delay_ms(CHECK_INTERVAL);
		}

		/* set the print_flag if all ports up or timeout */
		if (all_ports_up == 1 || count == (MAX_CHECK_TIME - 1)) {
			print_flag = 1;
			printf("done\n");
		}
	}
}

void
dpdk_eth_stats_print(size_t portid)
{
	struct rte_eth_stats stats;
	if (!rte_eth_stats_get(portid, &stats)) {
		printf("Port %lu Ethernet device stats\n", portid);
		printf("                TX         RX\n");
		printf("Success %10lu %10lu\n", stats.opackets, stats.ipackets);
		printf("  Bytes %10lu %10lu\n", stats.obytes, stats.ibytes);
		printf("Dropped %21lu\n", stats.imissed);
		printf(" Errors %10lu %10lu\n", stats.oerrors, stats.ierrors);
		printf("RX mbuf %21lu\n", stats.rx_nombuf);
	} else {
		printf("Error reading IO stats from Ethernet device on port %lu\n", portid);
	}
}
