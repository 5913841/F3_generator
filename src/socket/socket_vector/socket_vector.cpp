#include "socket/socket_vector/socket_vector.h"

SocketVector::SocketVector()
{

}

SocketVector::SocketVector(int size)
{
    sockets.resize(size);
}

void SocketVector::add_socket(const Socket& socket)
{
    sockets.push_back(socket);
}

Socket& SocketVector::get_socket(int index)
{
    return sockets[index];
}

int SocketVector::size() const
{
    return sockets.size();
}