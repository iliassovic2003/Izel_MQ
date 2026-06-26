#ifndef     METRICS_H
# define    METRICS_H

# include   <stdio.h>
# include   <stdlib.h>
# include   <string.h>
# include   <unistd.h>
# include   <pthread.h>
# include   "../core/broker/broker.h"
# include   "../core/message/message.h"


# define MAX_INTERFACES     16
# define MAX_CORES          32

typedef struct {
    unsigned long long  user;
    unsigned long long  nice;
    unsigned long long  system;
    unsigned long long  idle;
    unsigned long long  iowait;
    unsigned long long  irq;
    unsigned long long  softirq;
    unsigned long long  steal;
}                       cpu_stat_t;

typedef struct {
    unsigned long   total;
    unsigned long   free;
    unsigned long   available;
    unsigned long   buffers;
    unsigned long   cached;
    unsigned long   swap_total;
    unsigned long   swap_free;
    unsigned long   swap_cached;
    unsigned long   active;
    unsigned long   inactive;
    unsigned long   dirty;
}                   mem_stat_t;

typedef struct {
    char                name[32];
    unsigned long long  rx_bytes;
    unsigned long long  rx_packets;
    unsigned long long  rx_errors;
    unsigned long long  rx_dropped;
    unsigned long long  tx_bytes;
    unsigned long long  tx_packets;
    unsigned long long  tx_errors;
    unsigned long long  tx_dropped;
}                       net_iface_t;

typedef struct {
    net_iface_t     ifaces[MAX_INTERFACES];
    int             count;
}                   net_stat_t;


void    *producer_cpu(void *arg);
void    *producer_memory(void *arg);
void    *producer_network(void *arg);

#endif