#include "socket/socket_table/socket_table.h"

Socket *socket_ft = nullptr;

Socket *SocketPointerTable::find_socket(FiveTuples ft)
{
    if (unlikely(!socket_ft))
    {
        socket_ft = new Socket(ft);
    }
    else
    {
        socket_ft->dst_addr = ft.dst_addr;
        socket_ft->dst_port = ft.dst_port;
        socket_ft->src_addr = ft.src_addr;
        socket_ft->src_port = ft.src_port;
        ((NamedProtocol *)(socket_ft->l2_protocol))->code_ = ft.protocol_codes[SOCKET_L2];
        ((NamedProtocol *)(socket_ft->l3_protocol))->code_ = ft.protocol_codes[SOCKET_L3];
        ((NamedProtocol *)(socket_ft->l4_protocol))->code_ = ft.protocol_codes[SOCKET_L4];
        ((NamedProtocol *)(socket_ft->l5_protocol))->code_ = ft.protocol_codes[SOCKET_L5];
    }
    auto it = socket_table.find(socket_ft);
    if (it == socket_table.end())
    {
        return nullptr;
    }
    return *it;
}