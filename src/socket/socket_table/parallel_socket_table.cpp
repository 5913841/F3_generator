#include "socket/socket_table/parallel_socket_table.h"

thread_local Socket *p_socket_ft = nullptr;

Socket *ParallelSocketPointerTable::find_socket(FiveTuples ft)
{
    if (unlikely(!p_socket_ft))
    {
        p_socket_ft = new Socket(ft);
    }
    else
    {
        p_socket_ft->dst_addr = ft.dst_addr;
        p_socket_ft->dst_port = ft.dst_port;
        p_socket_ft->src_addr = ft.src_addr;
        p_socket_ft->src_port = ft.src_port;
        p_socket_ft->protocol = ft.protocol;
    }
    auto it = socket_table.find(p_socket_ft);
    if (it == socket_table.end())
    {
        return nullptr;
    }
    return *it;
}