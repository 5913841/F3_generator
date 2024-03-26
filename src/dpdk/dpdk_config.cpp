#include "dpdk/dpdk_config.h"
#include "panel/panel.h"
#include "timer/timer.h"

thread_local dpdk_config_percore *g_config_percore;
dpdk_config *g_config;

dpdk_config::dpdk_config(dpdk_config_user *user_config)
{
    num_lcores = user_config->lcores.size();
    memcpy(lcores, user_config->lcores.data(), num_lcores * sizeof(int));
    num_ports = user_config->ports.size();
    always_accurate_time = user_config->always_accurate_time;
    tx_burst_size = user_config->tx_burst_size;
    rx_burst_size = user_config->rx_burst_size;
    for (int i = 0; i < user_config->ports.size(); i++)
    {
        memcpy(ports[i].pci, user_config->ports[i].data(), PCI_LEN * sizeof(char));
        ports[i].pci_num = 1;
    }
    for (int i = 0; i < user_config->queue_num_per_port.size(); i++)
    {
        ports[i].queue_num = user_config->queue_num_per_port[i];
    }
    for (int i = 0; i < user_config->gateway_for_ports.size(); i++)
    {
        std::string type = distinguish_address_type(user_config->gateway_for_ports[i]);
        if (type == "IPv4")
        {
            ports[i].gateway_ipv6 = false;
            inet_aton(user_config->gateway_for_ports[i].data(), &ports[i].gateway_ip.ip);
        }
        else if (type == "IPv6")
        {
            ports[i].gateway_ipv6 = true;
            inet_pton(AF_INET6, user_config->gateway_for_ports[i].data(), &ports[i].gateway_ip.in6);
        }
        else if (type == "MAC Address")
        {
            memcpy(&ports[i].gateway_mac, ether_aton(user_config->gateway_for_ports[i].data()), sizeof(ether_addr));
        }
        else
        {
            printf("Error: Invalid gateway address type\n");
            exit(1);
        }
    }
    tick_init(0);
    // g_config_percore = new dpdk_config_percore(this);
    g_config = this;
}

dpdk_config_percore::dpdk_config_percore(dpdk_config *config)
{
    lcore_id = rte_lcore_id();
    port_id = lcore_id % config->num_ports;
    queue_id = lcore_id % config->num_ports;
    tick_time_init(&time);
    cpuload_init(&cpusage);
    port = &config->ports[port_id];
    rq = new rx_queue();
    tq = new tx_queue();
    tq->tx_burst = config->tx_burst_size;
    rq->rx_burst = config->rx_burst_size;
}

bool dpdk_config_percore::check_epoch_timer(uint64_t gap_tsc)
{
    tick_time_update(&g_config_percore->time);
    CPULOAD_ADD_TSC(&g_config_percore->cpusage, time_in_config(), g_config_percore->epoch_work);

    if (unlikely(tsc_time_go(&g_config_percore->time.second, time_in_config())))
    {
        net_stats_timer_handler();
        net_stats_print_speed(0, g_config_percore->time.second.count);
    }

    if (unlikely(time_in_config() - g_config_percore->epoch_last_tsc >= gap_tsc))
    {
        g_config_percore->epoch_work += 1;
        g_config_percore->epoch_last_tsc = time_in_config();
        return true;
    }

    return false;
}

uint64_t& time_in_config() {
    if(g_config->always_accurate_time) 
    {
        tick_time_update(&g_config_percore->time);
    }
    return g_config_percore->time.tsc;
}