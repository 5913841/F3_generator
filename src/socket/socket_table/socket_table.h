#ifndef SOCKET_TABLE_H
#define SOCKET_TABLE_H

#include "socket/socket.h"
#include <parallel_hashmap/phmap.h>

class SocketPointerTable
{
public:
    SocketPointerTable() {}
    SocketPointerTable(int init_size)
    {
        socket_table.reserve(init_size);
    }

    phmap::parallel_flat_hash_set<Socket *, SocketPointerHash, SocketPointerEqual> socket_table;
    // phmap::parallel_flat_hash_set<Socket *, SocketPointerHash, SocketPointerEqual>::iterator last_find_socket_table_it;
    Socket *find_socket(Socket *socket)
    {
        auto it = socket_table.find(socket);
        if (it == socket_table.end())
        {
            return nullptr;
        }
        // last_find_socket_table_it = it;
        return *it;
    }

    Socket *find_socket(Socket *socket, size_t hash_value)
    {
        auto it = socket_table.find(socket, hash_value);
        if (it == socket_table.end())
        {
            return nullptr;
        }
        // last_find_socket_table_it = it;
        return *it;
    }

    Socket *find_socket(FiveTuples ft);

    int insert_socket(Socket *socket)
    {
        auto it = socket_table.insert(socket);
        if (it.second == false)
        {
            return -1;
        }
        return 0;
    }

    int insert_socket(Socket *socket, size_t hash_value)
    {
        auto it = socket_table.emplace_with_hash(hash_value, socket);
        if(it.second == false)
        {
            return -1;
        }
        return 0;
    }

    int remove_socket(Socket *socket)
    {
        size_t num = socket_table.erase(socket);
        if (num == 0)
        {
            return -1;
        }
        return 0;
    }

    int remove_socket(Socket *socket, size_t hash_value)
    {
        auto it = socket_table.find(socket, hash_value);
        if (it == socket_table.end())
        {
            return -1;
        }
        // last_find_socket_table_it = it;
        socket_table.erase(it);
        return 0;
    }
};

#endif