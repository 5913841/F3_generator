#ifndef SOCKET_VECTOR_H
#define SOCKET_VECTOR_H

#include <vector>
#include "socket/socket.h"

class SocketVector
{
public:
    std::vector<Socket> sockets;
    SocketVector();
    SocketVector(int size);
    void add_socket(const Socket& socket);
    Socket& get_socket(int index);
    int size() const;
};

#endif /* SOCKET_VECTOR_H */