#ifndef SOCKET_RANGE_H
#define SOCKET_RANGE_H

#include "socket/socket.h"
#include "socket/ftrange.h"

struct SocketRangeTable {
    Socket* socket_table;
    bool* valid;
    FTRange range;


    SocketRangeTable(FTRange range_) : range(range_)
    {
        socket_table = new Socket[range.total_num];
        valid = new bool[range.total_num];
    }

    int insert_socket(Socket*& socket)
    {
        int idx = get_range_idx_ft(*socket, range);
        if (valid[idx]) {
            return -1;
        }
        valid[idx] = true;
        socket_table[idx] = *socket;
        delete socket;
        socket = &socket_table[idx];
        return 0;
    }

    Socket *find_socket(Socket *socket)
    {
        int idx = get_range_idx_ft(*socket, range);
        if (!valid[idx]) {
            return nullptr;
        }
        return &socket_table[get_range_idx_ft(*socket, range)];
    }

    int remove_socket(Socket *socket)
    {
        int idx = get_range_idx_ft(*socket, range);
        if (!valid[idx]) {
            return -1;
        }
        valid[idx] = false;
        return 0;
    }
};

#endif