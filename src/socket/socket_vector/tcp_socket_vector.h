#ifndef TCP_SOCKET_VECTOR_H
#define TCP_SOCKET_VECTOR_H

#include <vector>
#include <string>
#include "socket/socket.h"
#include "protocols/TCP.h"
#include "protocols/HTTP.h"

SocketPointerEqual point_eq;

class TCPSocketVector
{
public:
    std::vector<Socket> sockets;
    std::vector<TCP> tcp_protocols;
    std::vector<HTTP> http_protocols;
    TCPSocketVector() {}
    TCPSocketVector(size_t size)
    {
        sockets.resize(size);
        tcp_protocols.resize(size);
        http_protocols.resize(size);
    }
    virtual ~TCPSocketVector() {}
    Socket *new_socket(Socket *template_socket)
    {
        sockets.emplace(sockets.end(), *template_socket);
        tcp_protocols.emplace(tcp_protocols.end(), *(TCP *)template_socket->l4_protocol);
        http_protocols.emplace(http_protocols.end(), *(HTTP *)template_socket->l5_protocol);

        return &sockets.back();
    }
    Socket *find_socket(Socket *socket)
    {
        auto it = sockets.begin();
        for (; it != sockets.end(); ++it)
        {
            if (point_eq(&*it, socket))
            {
                return &*it;
            }
        }
        if (it == sockets.end())
        {
            return nullptr;
        }
    }
    void remove_socket(Socket *socket)
    {
        auto it = sockets.begin();
        for (; it != sockets.end(); ++it)
        {
            if (point_eq(&*it, socket))
            {
                sockets.erase(it);
                tcp_protocols.erase(tcp_protocols.begin() + (it - sockets.begin()));
                http_protocols.erase(http_protocols.begin() + (it - sockets.begin()));
                break;
            }
        }
    }
    void clear()
    {
        sockets.clear();
        tcp_protocols.clear();
        http_protocols.clear();
    }
};

#endif // TCP_SOCKET_VECTOR_H