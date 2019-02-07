#include "dpdk.h"

#include "util.h"       // for the debug and todo stuff
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int dpdk_read(Location l,  Addr data, size_t size){
    return 0;
}
//------------------------------------------------------------------------------
int dpdk_write(Location l,  Addr data, size_t size){
    return 0;
}
//------------------------------------------------------------------------------
int dpdk_query(Location l, DPDK_QUERY_TYPE t ){
    return 0;
}
//------------------------------------------------------------------------------
Addr    dpdk_alloc(Location l, size_t size){
    return NULL;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int dpdk_shell(int argc, char *argv[]) {
    PRINTF("argc %d argv 0x%08lX\n", argc, (size_t)argv);

    for(;;) {
        printf("== MENU ==\n"
               "[send] 21: pull  22: push  23: query\n"
               "30: read_random\n"
               "40: write_random\n"
               "[message] 51: randomize  52: push  53: pull  54: compare\n"
               "59: very_one_write_read\n"
               "[stats] 91: init  99: print\n"
               "999: Quit\n"
               "> ");
        int command;
        int count;
        int port;
        int value;
        (void)scanf("%d", &command);

        switch (command) {
        case 21:
        case 53:
            printf("port> ");
            (void)scanf("%d", &port);
            printf("index> ");
            (void)scanf("%d", &count);
            //send_pull(port, count);
            break;
        case 22:
            printf("port> ");
            (void)scanf("%d", &port);
            printf("index> ");
            (void)scanf("%d", &count);
            printf("value> ");
            (void)scanf("%d", &value);
            //send_push(port, count, value);
            break;
        case 23:
            printf("port> ");
            (void)scanf("%d", &port);
            //send_query(port);
            break;
        case 30:
            printf("port> ");
            (void)scanf("%d", &port);
            printf("burst delay> ");
            (void)scanf("%d", &count);
            //read_random(port, count);
            break;
        case 40:
            printf("port> ");
            (void)scanf("%d", &port);
            printf("burst delay> ");
            (void)scanf("%d", &count);
            //write_random(port, count);
            break;
        case 51: // randomize message, push message, pull message,  compare
            //randomize_message();
            break;
        case 52:
            printf("port> ");
            (void)scanf("%d", &port);
            printf("index> ");
            (void)scanf("%d", &count);
            //push_message(port, count);
            break;
        case 54:
            printf("index> ");
            (void)scanf("%d", &count);
            //compare_message(count);
            break;

        case 91:
            //dpdk_init_port_stats();
//            [[fallthrough]];
        case 99:
            //dpdk_print_port_stats_all();
            break;

        case 999:
            printf("Quitting...\n");
            return 0;
        }
    }

    return 0;
}