#ifndef FTRANGE_H
#define FTRANGE_H

#include "socket/socket.h"

struct FTRange {
    ip4addr_t start_src_ip;
    ip4addr_t start_dst_ip;
    port_t start_src_port;
    port_t start_dst_port;
    ip4addr_t end_src_ip;
    ip4addr_t end_dst_ip;
    port_t end_src_port;
    port_t end_dst_port;
    int src_ip_num;
    int dst_ip_num;
    int src_port_num;
    int dst_port_num;
    int total_num;
};

inline void init_ft_range(FTRange* ft_range)
{
    ft_range->end_dst_ip = ft_range->start_dst_ip + ft_range->dst_ip_num - 1;
    ft_range->end_src_ip = ft_range->start_src_ip + ft_range->src_ip_num - 1;
    ft_range->end_dst_port = ft_range->start_dst_port + ft_range->dst_port_num - 1;
    ft_range->end_src_port = ft_range->start_src_port + ft_range->src_port_num - 1;
    ft_range->total_num = ft_range->src_ip_num * ft_range->dst_ip_num * ft_range->src_port_num * ft_range->dst_port_num;
}

inline void set_start_ft(Socket* socket, const FTRange& ft_range)
{
    socket->src_addr = ft_range.start_src_ip;
    socket->dst_addr = ft_range.start_dst_ip;
    socket->src_port = ft_range.start_src_port;
    socket->dst_port = ft_range.start_dst_port;
}

inline void increase_ft(Socket* socket, const FTRange& ft_range)
{
    socket->dst_port = socket->dst_port + 1;
    if (socket->dst_port > ft_range.end_dst_port) {
        socket->dst_port = ft_range.start_dst_port;
        socket->dst_addr = socket->dst_addr + 1;
        if (socket->dst_addr > ft_range.end_dst_ip) {
            socket->dst_addr = ft_range.start_dst_ip;
            socket->src_port = socket->src_port + 1;
            if (socket->src_port > ft_range.end_src_port) {
                socket->src_port = ft_range.start_src_port;
                socket->src_addr = socket->src_addr + 1;
                if (socket->src_addr > ft_range.end_src_ip) {
                    socket->src_addr = ft_range.start_src_ip;
                }
            }
        }
    }
}

inline void set_by_range_idx_ft(Socket* socket, const FTRange& ft_range, int idx)
{
    int dst_port_idx = idx % ft_range.dst_port_num;
    idx -= dst_port_idx; idx /= ft_range.dst_port_num;
    int dst_ip_idx = idx % ft_range.dst_ip_num;
    idx -= dst_ip_idx; idx /= ft_range.dst_ip_num;
    int src_port_idx = idx % ft_range.src_port_num;
    idx -= src_port_idx; idx /= ft_range.src_port_num;
    int src_ip_idx = idx;

    socket->src_addr = ft_range.start_src_ip + src_ip_idx;
    socket->dst_addr = ft_range.start_dst_ip + dst_ip_idx;
    socket->src_port = ft_range.start_src_port + src_port_idx;
    socket->dst_port = ft_range.start_dst_port + dst_port_idx;
}

inline int get_range_idx_ft(const Socket& socket, const FTRange& ft_range)
{
    int src_ip_idx = socket.src_addr - ft_range.start_src_ip;
    int dst_ip_idx = socket.dst_addr - ft_range.start_dst_ip;
    int src_port_idx = socket.src_port - ft_range.start_src_port;
    int dst_port_idx = socket.dst_port - ft_range.start_dst_port;

    return ((src_ip_idx * ft_range.src_port_num + src_port_idx) * ft_range.dst_ip_num + dst_ip_idx) * ft_range.dst_port_num + dst_port_idx;
}

#endif