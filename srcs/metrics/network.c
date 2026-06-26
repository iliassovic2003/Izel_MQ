#include "metrics.h"

static void read_netdev(net_stat_t *stat)
{
    FILE    *f;
    char    line[512];
    char    name[32];
    int     count = 0;

    memset(stat, 0, sizeof(*stat));
    f = fopen("/proc/net/dev", "r");
    if (!f)
        return;

    fgets(line, sizeof(line), f);
    fgets(line, sizeof(line), f);

    while (fgets(line, sizeof(line), f) && count < MAX_INTERFACES)
    {
        net_iface_t *iface = &stat->ifaces[count];

        if (sscanf(line,
            " %31[^:]: %llu %llu %llu %llu %*u %*u %*u %*u"
            " %llu %llu %llu %llu",
            name,
            &iface->rx_bytes,   &iface->rx_packets,
            &iface->rx_errors,  &iface->rx_dropped,
            &iface->tx_bytes,   &iface->tx_packets,
            &iface->tx_errors,  &iface->tx_dropped) == 9)
        {
            if (strcmp(name, "lo") == 0)
                continue;
            strncpy(iface->name, name, sizeof(iface->name) - 1);
            count++;
        }
    }
    stat->count = count;
    fclose(f);
}

void    *producer_network(void *arg)
{
    izq_broker_t    *broker = (izq_broker_t *)arg;
    net_stat_t      prev;
    net_stat_t      curr;
    char            payload[IMQ_DATA_PAYLOAD_MAX];
    char            ifaces_json[1024];
    imq_message_t   *msg;
    int             offset;

    read_netdev(&prev);
    sleep(1);

    while (1)
    {
        read_netdev(&curr);

        offset = 0;
        offset += snprintf(ifaces_json + offset,
                    sizeof(ifaces_json) - offset, "[");

        for (int i = 0; i < curr.count; i++)
        {
            net_iface_t *p = NULL;
            for (int j = 0; j < prev.count; j++)
            {
                if (strcmp(prev.ifaces[j].name, curr.ifaces[i].name) == 0)
                {
                    p = &prev.ifaces[j];
                    break;
                }
            }

            unsigned long long rx_rate = p
                ? curr.ifaces[i].rx_bytes - p->rx_bytes : 0;
            unsigned long long tx_rate = p
                ? curr.ifaces[i].tx_bytes - p->tx_bytes : 0;

            offset += snprintf(ifaces_json + offset,
                        sizeof(ifaces_json) - offset,
                        "%s{"
                        "\"name\":\"%s\","
                        "\"rx_bytes\":%llu,"
                        "\"tx_bytes\":%llu,"
                        "\"rx_rate_bps\":%llu,"
                        "\"tx_rate_bps\":%llu,"
                        "\"rx_packets\":%llu,"
                        "\"tx_packets\":%llu,"
                        "\"rx_errors\":%llu,"
                        "\"tx_errors\":%llu,"
                        "\"rx_dropped\":%llu,"
                        "\"tx_dropped\":%llu"
                        "}",
                        i > 0 ? "," : "",
                        curr.ifaces[i].name,
                        curr.ifaces[i].rx_bytes,
                        curr.ifaces[i].tx_bytes,
                        rx_rate,
                        tx_rate,
                        curr.ifaces[i].rx_packets,
                        curr.ifaces[i].tx_packets,
                        curr.ifaces[i].rx_errors,
                        curr.ifaces[i].tx_errors,
                        curr.ifaces[i].rx_dropped,
                        curr.ifaces[i].tx_dropped);
        }

        offset += snprintf(ifaces_json + offset,
                    sizeof(ifaces_json) - offset, "]");

        snprintf(payload, sizeof(payload),
            "{"
            "\"interface_count\":%d,"
            "\"interfaces\":%s"
            "}",
            curr.count,
            ifaces_json);

        msg = imq_msg_create(IMQ_DATA, "metrics.network",
                payload, strlen(payload) + 1);
        if (msg)
            broker_publish(broker, "metrics.network", msg);

        prev = curr;
        sleep(1);
    }
    return NULL;
}