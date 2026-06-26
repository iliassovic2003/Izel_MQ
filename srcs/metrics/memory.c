#include "metrics.h"

static void read_meminfo(mem_stat_t *stat)
{
    FILE    *f;
    char    key[64];
    unsigned long value;

    memset(stat, 0, sizeof(*stat));
    f = fopen("/proc/meminfo", "r");
    if (!f)
        return;

    while (fscanf(f, "%63s %lu kB\n", key, &value) == 2)
    {
        if      (strcmp(key, "MemTotal:")     == 0) stat->total      = value;
        else if (strcmp(key, "MemFree:")      == 0) stat->free       = value;
        else if (strcmp(key, "MemAvailable:") == 0) stat->available  = value;
        else if (strcmp(key, "Buffers:")      == 0) stat->buffers    = value;
        else if (strcmp(key, "Cached:")       == 0) stat->cached     = value;
        else if (strcmp(key, "SwapTotal:")    == 0) stat->swap_total = value;
        else if (strcmp(key, "SwapFree:")     == 0) stat->swap_free  = value;
        else if (strcmp(key, "SwapCached:")   == 0) stat->swap_cached= value;
        else if (strcmp(key, "Active:")       == 0) stat->active     = value;
        else if (strcmp(key, "Inactive:")     == 0) stat->inactive   = value;
        else if (strcmp(key, "Dirty:")        == 0) stat->dirty      = value;
    }
    fclose(f);
}

void    *producer_memory(void *arg)
{
    izq_broker_t    *broker = (izq_broker_t *)arg;
    mem_stat_t      stat;
    char            payload[IMQ_DATA_PAYLOAD_MAX];
    imq_message_t   *msg;
    double          used_pct;
    double          swap_pct;
    unsigned long   used;
    unsigned long   swap_used;

    while (1)
    {
        read_meminfo(&stat);

        used      = stat.total - stat.available;
        swap_used = stat.swap_total - stat.swap_free;
        used_pct  = stat.total > 0
                    ? 100.0 * (double)used / (double)stat.total
                    : 0.0;
        swap_pct  = stat.swap_total > 0
                    ? 100.0 * (double)swap_used / (double)stat.swap_total
                    : 0.0;

        snprintf(payload, sizeof(payload),
            "{"
            "\"total_kb\":%lu,"
            "\"free_kb\":%lu,"
            "\"available_kb\":%lu,"
            "\"used_kb\":%lu,"
            "\"used_pct\":%.1f,"
            "\"buffers_kb\":%lu,"
            "\"cached_kb\":%lu,"
            "\"active_kb\":%lu,"
            "\"inactive_kb\":%lu,"
            "\"dirty_kb\":%lu,"
            "\"swap_total_kb\":%lu,"
            "\"swap_used_kb\":%lu,"
            "\"swap_pct\":%.1f"
            "}",
            stat.total,
            stat.free,
            stat.available,
            used,
            used_pct,
            stat.buffers,
            stat.cached,
            stat.active,
            stat.inactive,
            stat.dirty,
            stat.swap_total,
            swap_used,
            swap_pct);

        msg = imq_msg_create(IMQ_DATA, "metrics.memory",
                payload, strlen(payload) + 1);
        if (msg)
            broker_publish(broker, "metrics.memory", msg);

        sleep(1);
    }
    return NULL;
}