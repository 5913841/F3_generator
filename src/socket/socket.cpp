#include "socket/socket.h"

ipaddr_t::ipaddr_t(const ip4addr_t& other)
{
    ip = other.ip;
}

bool FiveTuples::operator==(const FiveTuples &other) const
{
    // return likely(src_addr == other.src_addr &&
    //     dst_addr == other.dst_addr &&
    //     src_port == other.src_port &&
    //     dst_port == other.dst_port &&
    //     protocol_codes[0] == other.protocol_codes[0] &&
    //     protocol_codes[1] == other.protocol_codes[1] &&
    //     protocol_codes[2] == other.protocol_codes[2] &&
    //     protocol_codes[3] == other.protocol_codes[3]);
    return memcmp(this, &other, sizeof(FiveTuples)) == 0;
}


FiveTuples::FiveTuples(const Socket &socket)
{
    memset(this, 0, sizeof(FiveTuples));
    src_addr = socket.src_addr;
    dst_addr = socket.dst_addr;
    src_port = socket.src_port;
    dst_port = socket.dst_port;
    protocol = socket.protocol;
}

size_t FiveTuples::hash(const FiveTuples &ft)
{
    size_t hash_value = 0x12345678;
    hash_value += std::hash<uint32_t>()(ft.src_addr.ip.s_addr);
    hash_value += std::hash<uint32_t>()(ft.dst_addr.ip.s_addr);
    hash_value += std::hash<uint16_t>()(ft.src_port);
    hash_value += std::hash<uint16_t>()(ft.dst_port);
    hash_value += std::hash<uint8_t>()(ft.protocol);
    return hash_value;
}

size_t Socket::hash(const Socket* sk)
{
    size_t hash_value = 0x12345678;
    if (sk) {
        hash_value += std::hash<uint32_t>()(sk->src_addr.ip.s_addr);
        hash_value += std::hash<uint32_t>()(sk->dst_addr.ip.s_addr);
        hash_value += std::hash<uint16_t>()(sk->src_port);
        hash_value += std::hash<uint16_t>()(sk->dst_port);
        hash_value += std::hash<uint8_t>()(sk->protocol);
    }
    return hash_value;
}


size_t Socket::hash(const Socket &sk)
{
    size_t hash_value = 0x12345678;
    hash_value += std::hash<uint32_t>()(sk.src_addr.ip.s_addr);
    hash_value += std::hash<uint32_t>()(sk.dst_addr.ip.s_addr);
    hash_value += std::hash<uint16_t>()(sk.src_port);
    hash_value += std::hash<uint16_t>()(sk.dst_port);
    hash_value += std::hash<uint8_t>()(sk.protocol);
    return hash_value;
}