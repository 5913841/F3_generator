#ifndef SOCKET_PRANGE_H
#define SOCKET_PRANGE_H

#include "socket/socket.h"
#include "socket/ftrange.h"

struct SocketPointerRangeTable {
    Socket** socket_table;
    FTRange range;


    SocketPointerRangeTable(FTRange range_) : range(range_)
    {
        socket_table = new Socket*[range.total_num];
        memset(socket_table, 0, sizeof(Socket*) * range.total_num);
    }

    int insert_socket(Socket *socket)
    {
        int idx = get_range_idx_ft(*socket, range);
        if (socket_table[idx] == nullptr)
        {
            socket_table[idx] = socket;
            return 0;
        }
        else
        {
            return -1;
        }
    }

    Socket *find_socket(Socket *socket)
    {
        return socket_table[get_range_idx_ft(*socket, range)];
    }

    int remove_socket(Socket *socket)
    {
        int idx = get_range_idx_ft(*socket, range);
        if (socket_table[idx] == nullptr)
        {
            return -1;
        }
        else
        {
            socket_table[idx] = nullptr;
            return 0;
        }
    }
};

#endif