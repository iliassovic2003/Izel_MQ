#include "metrics.h"

static int  read_cpu_stat(const char *label, cpu_stat_t *stat)
{
    FILE    *f;
    char    line[256];
    char    cpu_label[16];

    f = fopen("/proc/stat", "r");
    if (!f)
        return 0;
    while (fgets(line, sizeof(line), f))
    {
        if (sscanf(line, "%15s %llu %llu %llu %llu %llu %llu %llu %llu",
            cpu_label,
            &stat->user, &stat->nice, &stat->system, &stat->idle,
            &stat->iowait, &stat->irq, &stat->softirq, &stat->steal) == 9)
        {
            if (strcmp(cpu_label, label) == 0)
            {
                fclose(f);
                return 1;
            }
        }
    }
    fclose(f);
    return 0;
}

static double   calc_cpu_percent(cpu_stat_t *prev, cpu_stat_t *curr)
{
    unsigned long long  prev_total;
    unsigned long long  curr_total;
    unsigned long long  prev_idle;
    unsigned long long  curr_idle;
    unsigned long long  total_diff;
    unsigned long long  idle_diff;

    prev_idle  = prev->idle + prev->iowait;
    curr_idle  = curr->idle + curr->iowait;
    prev_total = prev->user + prev->nice + prev->system + prev_idle
                    + prev->irq + prev->softirq + prev->steal;
    curr_total = curr->user + curr->nice + curr->system + curr_idle
                    + curr->irq + curr->softirq + curr->steal;

    total_diff = curr_total - prev_total;
    idle_diff  = curr_idle  - prev_idle;

    if (total_diff == 0)
        return 0.0;
    return 100.0 * (double)(total_diff - idle_diff) / (double)total_diff;
}

static int  count_cores(void)
{
    FILE    *f;
    char    line[256];
    int     cores = 0;

    f = fopen("/proc/stat", "r");
    if (!f)
        return 1;
    while (fgets(line, sizeof(line), f))
        if (strncmp(line, "cpu", 3) == 0 && line[3] != ' ')
            cores++;
    fclose(f);
    return cores;
}

static float    read_load_avg(int which)
{
    FILE    *f;
    float   a, b, c;

    f = fopen("/proc/loadavg", "r");
    if (!f)
        return 0.0f;
    fscanf(f, "%f %f %f", &a, &b, &c);
    fclose(f);
    if (which == 0) return a;
    if (which == 1) return b;
    return c;
}

void    *producer_cpu(void *arg)
{
    izq_broker_t    *broker = (izq_broker_t *)arg;
    cpu_stat_t      prev[MAX_CORES + 1];
    cpu_stat_t      curr[MAX_CORES + 1];
    char            label[16];
    char            payload[IMQ_DATA_PAYLOAD_MAX];
    char            cores_json[768];
    int             num_cores;
    int             offset;
    imq_message_t   *msg;

    num_cores = count_cores();

    read_cpu_stat("cpu", &prev[0]);
    for (int i = 0; i < num_cores; i++)
    {
        snprintf(label, sizeof(label), "cpu%d", i);
        read_cpu_stat(label, &prev[i + 1]);
    }
    sleep(1);

    while (1)
    {
        read_cpu_stat("cpu", &curr[0]);
        for (int i = 0; i < num_cores; i++)
        {
            snprintf(label, sizeof(label), "cpu%d", i);
            read_cpu_stat(label, &curr[i + 1]);
        }

        double overall = calc_cpu_percent(&prev[0], &curr[0]);

        offset = 0;
        offset += snprintf(cores_json + offset,
                    sizeof(cores_json) - offset, "[");
        for (int i = 0; i < num_cores; i++)
        {
            double core_pct = calc_cpu_percent(&prev[i + 1], &curr[i + 1]);
            offset += snprintf(cores_json + offset,
                        sizeof(cores_json) - offset,
                        "%s%.1f", i > 0 ? "," : "", core_pct);
        }
        offset += snprintf(cores_json + offset,
                    sizeof(cores_json) - offset, "]");

        snprintf(payload, sizeof(payload),
            "{"
            "\"cpu_pct\":%.1f,"
            "\"cores\":%d,"
            "\"per_core\":%s,"
            "\"load_1\":%.2f,"
            "\"load_5\":%.2f,"
            "\"load_15\":%.2f"
            "}",
            overall,
            num_cores,
            cores_json,
            read_load_avg(0),
            read_load_avg(1),
            read_load_avg(2));

        msg = imq_msg_create(IMQ_DATA, "metrics.cpu",
                payload, strlen(payload) + 1);
        if (msg)
            broker_publish(broker, "metrics.cpu", msg);

        for (int i = 0; i <= num_cores; i++)
            prev[i] = curr[i];

        sleep(1);
    }
    return NULL;
}