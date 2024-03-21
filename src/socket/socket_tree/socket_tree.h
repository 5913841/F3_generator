#ifndef SOCKET_TREE_H
#define SOCKET_TREE_H

#include "socket/socket.h"
#include <map>

class SocketTree {
public:
    std::map<FiveTuples, Socket*> socket_tree;


    SocketTree() {}

    Socket* find_socket(FiveTuples five_tuple) {
        auto it = socket_tree.find(five_tuple);
        if (it == socket_tree.end()) {
            return nullptr; // not found
        }
        return it->second;
    }


    void insert_socket(Socket* socket) {
        auto it = socket_tree.insert(std::make_pair(FiveTuples(*socket), socket));
        if (!it.second) {
            throw std::runtime_error("Socket already exists in the tree.");
        }
    }

    void remove_socket(Socket* socket) {
        auto num = socket_tree.erase(FiveTuples(*socket));
        delete socket;
        if (num == 0) {
            throw std::runtime_error("Socket not found in the tree.");
        }
    }
};

#endif /* SOCKET_TREE_H */