/*
 * Copyright (c) 2021 Baidu.com, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jianzhang Peng (pengjianzhang@baidu.com)
 */

/*
 * client
 * ------------------------------------------------
 * each client thread sends requests to one dest address (server)
 * each client thread use one tx/rx queue
 *
 * server
 * ------------------------------------------------
 * each server thread binds one ip address
 * each server thread use one tx/rx queue
 *
 * */

#include "dpdk/divert/fdir.h"

#include <stdint.h>
#include <rte_flow.h>

#include "dpdk/dpdk_config.h"
#include <cmath>

static void fdir_pattern_init_eth(struct rte_flow_item *pattern, struct rte_flow_item_eth *spec,
                                  struct rte_flow_item_eth *mask)
{
    memset(spec, 0, sizeof(struct rte_flow_item_eth));
    memset(mask, 0, sizeof(struct rte_flow_item_eth));
    spec->type = 0;
    mask->type = 0;

    memset(pattern, 0, sizeof(struct rte_flow_item));
    pattern->type = RTE_FLOW_ITEM_TYPE_ETH;
    pattern->spec = spec;
    pattern->mask = mask;
}

static void fdir_pattern_init_ipv4(struct rte_flow_item *pattern, struct rte_flow_item_ipv4 *spec,
                                   struct rte_flow_item_ipv4 *mask, uint32_t smask, uint32_t dmask, uint32_t sip, uint32_t dip)
{
    memset(spec, 0, sizeof(struct rte_flow_item_ipv4));
    spec->hdr.dst_addr = dip;
    spec->hdr.src_addr = sip;

    memset(mask, 0, sizeof(struct rte_flow_item_ipv4));
    mask->hdr.dst_addr = dmask;
    mask->hdr.src_addr = smask;

    memset(pattern, 0, sizeof(struct rte_flow_item));
    pattern->type = RTE_FLOW_ITEM_TYPE_IPV4;
    pattern->spec = spec;
    pattern->mask = mask;
}

static void fdir_pattern_init_ipv6(struct rte_flow_item *pattern, struct rte_flow_item_ipv6 *spec,
                                   struct rte_flow_item_ipv6 *mask, ipaddr_t sip, ipaddr_t dip)
{
    uint32_t mask_zero[4] = {0, 0, 0, 0};
    uint32_t mask_full[4] = {0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff};
    uint32_t *smask = NULL;
    uint32_t *dmask = NULL;

    if (sip.ip.s_addr != 0)
    {
        smask = mask_full;
        dmask = mask_zero;
    }
    else
    {
        smask = mask_zero;
        dmask = mask_full;
    }

    memset(spec, 0, sizeof(struct rte_flow_item_ipv6));
    memcpy(spec->hdr.dst_addr, &dip, sizeof(struct in6_addr));
    memcpy(spec->hdr.src_addr, &sip, sizeof(struct in6_addr));

    memset(mask, 0, sizeof(struct rte_flow_item_ipv6));
    memcpy(mask->hdr.dst_addr, dmask, sizeof(struct in6_addr));
    memcpy(mask->hdr.src_addr, smask, sizeof(struct in6_addr));

    memset(pattern, 0, sizeof(struct rte_flow_item));
    pattern->type = RTE_FLOW_ITEM_TYPE_IPV6;
    pattern->spec = spec;
    pattern->mask = mask;
}

static void fdir_pattern_init_end(struct rte_flow_item *pattern)
{
    memset(pattern, 0, sizeof(struct rte_flow_item));
    pattern->type = RTE_FLOW_ITEM_TYPE_END;
}

static void fdir_action_init(struct rte_flow_action *action, struct rte_flow_action_queue *queue, uint16_t rxq)
{
    memset(action, 0, sizeof(struct rte_flow_action) * 2);
    memset(queue, 0, sizeof(struct rte_flow_action_queue));

    queue->index = rxq;
    action[0].type = RTE_FLOW_ACTION_TYPE_QUEUE;
    action[0].conf = queue;
    action[1].type = RTE_FLOW_ACTION_TYPE_END;
}

static int fdir_create(uint8_t port_id, struct rte_flow_item *pattern, struct rte_flow_action *action)
{
    int ret = 0;
    struct rte_flow *flow = NULL;
    struct rte_flow_error err;
    struct rte_flow_attr attr;

    memset(&attr, 0, sizeof(struct rte_flow_attr));
    attr.ingress = 1;

    // EINVAL
    ret = rte_flow_validate(port_id, &attr, pattern, action, &err);
    if (ret < 0)
    {
        printf("Error: Interface dose not support FDIR. Please use 'rss'!\n");
        return ret;
    }

    flow = rte_flow_create(port_id, &attr, pattern, action, &err);
    if (flow == NULL)
    {
        printf("Error: Flow create error\n");
        return -1;
    }

    return 0;
}

static int fdir_new(uint8_t port_id, uint16_t rxq, ipaddr_t smask, ipaddr_t dmask, ipaddr_t sip, ipaddr_t dip, bool ipv6)
{
    struct rte_flow_action_queue queue;
    struct rte_flow_item_eth eth_spec, eth_mask;
    struct rte_flow_item_ipv4 ip_spec, ip_mask;
    struct rte_flow_item_ipv6 ip6_spec, ip6_mask;
    struct rte_flow_item pattern[MAX_PATTERNS];
    struct rte_flow_action action[MAX_PATTERNS];

    fdir_action_init(action, &queue, rxq);
    fdir_pattern_init_eth(&pattern[0], &eth_spec, &eth_mask);
    if (ipv6)
    {
        fdir_pattern_init_ipv6(&pattern[1], &ip6_spec, &ip6_mask, sip, dip);
    }
    else
    {
        fdir_pattern_init_ipv4(&pattern[1], &ip_spec, &ip_mask, smask, dmask, sip.ip.s_addr, dip.ip.s_addr);
    }
    fdir_pattern_init_end(&pattern[2]);

    return fdir_create(port_id, pattern, action);
}

static int fdir_init_port(struct netif_port *port, bool ipv6, uint32_t src_range_mask, uint32_t dst_range_mask, uint32_t src_base_ip, uint32_t dst_base_ip)
{
    int ret = 0;
    int queue_id = 0;
    rte_flow_flush(port->id, NULL);
    uint32_t log_queue_num = 0;
    while ((1 << log_queue_num) < port->queue_num)
    {
        log_queue_num++;
    }
    for (queue_id = 0; queue_id < port->queue_num; queue_id++)
    {
        uint32_t valid_pos = 0;
        uint32_t *ths_src_mask = new uint32_t(src_range_mask);
        uint32_t *ths_dst_mask = new uint32_t(dst_range_mask);
        uint32_t *ths_src_addr = new uint32_t(src_base_ip);
        uint32_t *ths_dst_addr = new uint32_t(dst_base_ip);
        for (int i = 0; i < 64; i++)
        {
            uint32_t *r_mask = i % 2 ? ths_dst_mask : ths_src_mask;
            uint32_t *ip = i % 2 ? ths_dst_addr : ths_src_addr;
            uint32_t pos = i / 2;
            if (*r_mask & (1 << pos))
            {
                if (valid_pos < log_queue_num)
                {
                    *r_mask &= ~(1 << pos);
                    *ip &= ~(1 << pos);
                    *ip |= (queue_id & (1 << valid_pos)) >> valid_pos << pos;
                    valid_pos++;
                }
                // else
                // {
                //     *ip |= 1 << pos;
                // }
            }
        }
        // ret = fdir_new(port->id, queue_id, ~*ths_src_mask, ~*ths_dst_mask, *ths_src_addr, *ths_dst_addr, ipv6);
        ret = fdir_new(port->id, queue_id, 1, 0, 2, 0, ipv6);
#ifdef DEBUG_
        printf("fdir_init_port: port_id=%d, queue_id=%d, src_mask=%x, dst_mask=%x, src_base_ip=%x, dst_base_ip=%x, ret=%d\n", port->id, queue_id, ~*ths_src_mask, ~*ths_dst_mask, *ths_src_addr, *ths_dst_addr, ret);
#endif
        delete ths_src_mask;
        delete ths_dst_mask;
        delete ths_src_addr;
        delete ths_dst_addr;
        if (ret < 0)
        {
            return -1;
        }
    }
    return 0;
}

// static int fdir_init_vxlan(struct netif_port *port)
// {
//     int ret = 0;
//     int queue_id = 0;
//     bool ipv6 = false;  /* vxlan outer headers is ipv4 */
//     ipaddr_t local_ip;
//     ipaddr_t remote_ip;
//     struct vxlan *vxlan = NULL;

//     vxlan = port->vxlan;
//     memset(&remote_ip, 0, sizeof(ipaddr_t));
//     rte_flow_flush(port->id, NULL);
//     for (queue_id = 0; queue_id < port->queue_num; queue_id++) {
//         ip_range_get2(&vxlan->vtep_local, queue_id, &local_ip);

//         ret = fdir_new(port->id, queue_id, remote_ip, local_ip, ipv6);
//         if (ret < 0) {
//             return -1;
//         }
//     }

//     return 0;
// }

int fdir_init(struct dpdk_config *cfg)
{
    struct netif_port *port = NULL;
    bool ipv6 = cfg->addr_family == AF_INET6;

    config_for_each_port(cfg, port)
    {
        if (port->queue_num == 1)
        {
            continue;
        }
        // if (cfg->vxlan) {
        //     if (fdir_init_vxlan(port) < 0) {
        //         return -1;
        //     }
        // } else {
        if (fdir_init_port(port, ipv6, cfg->src_range_mask, cfg->dst_range_mask, cfg->src_base_ip, cfg->dst_base_ip) < 0)
        {
            return -1;
        }
        // }
    }

    return 0;
}

void fdir_flush(struct dpdk_config *cfg)
{
    struct netif_port *port = NULL;

    if ((g_config->num_lcores <= 1))
    {
        return;
    }

    config_for_each_port(cfg, port)
    {
        if (port->queue_num == 1)
        {
            continue;
        }

        rte_flow_flush(port->id, NULL);
    }
}
