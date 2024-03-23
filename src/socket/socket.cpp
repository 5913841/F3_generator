#include "socket/socket.h"
#include <hash_map>

bool FiveTuples::operator == (const FiveTuples& other) const
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

bool FiveTuples::operator < (const FiveTuples& other) const
{
    if(src_addr != other.src_addr)
    {
        return src_addr < other.src_addr;
    }
    if(dst_addr != other.dst_addr)
    {
        return dst_addr < other.dst_addr;
    }
    if(src_port != other.src_port)
    {
        return src_port < other.src_port;
    }
    if(dst_port != other.dst_port)
    {
        return dst_port < other.dst_port;
    }
    for(int i = 0; i < 4; i++)
    {
        if(protocol_codes[i] != other.protocol_codes[i])
        {
            return protocol_codes[i] < other.protocol_codes[i];
        }
    }
    return false;
}



size_t FiveTuples::hash(const FiveTuples& ft)
{
    size_t hash_value = 0x12345678;
    hash_value += std::hash<uint32_t>()(ft.src_addr.in6.s6_addr32[0]);
    hash_value += std::hash<uint32_t>()(ft.src_addr.in6.s6_addr32[1]);
    hash_value += std::hash<uint32_t>()(ft.src_addr.in6.s6_addr32[2]);
    hash_value += std::hash<uint32_t>()(ft.src_addr.in6.s6_addr32[3]);
    hash_value += std::hash<uint32_t>()(ft.dst_addr.in6.s6_addr32[0]);
    hash_value += std::hash<uint32_t>()(ft.dst_addr.in6.s6_addr32[1]);
    hash_value += std::hash<uint32_t>()(ft.dst_addr.in6.s6_addr32[2]);
    hash_value += std::hash<uint32_t>()(ft.dst_addr.in6.s6_addr32[3]);
    hash_value += std::hash<uint16_t>()(ft.src_port);
    hash_value += std::hash<uint16_t>()(ft.dst_port);
    for(int i = 0; i < 4; i++)
    {
        hash_value += std::hash<uint8_t>()(ft.protocol_codes[i]);
    }
    return hash_value;
}

template<typename l2_protocol, typename l3_protocol, typename l4_protocol, typename l5_protocol>
size_t Socket<l2_protocol, l3_protocol, l4_protocol, l5_protocol>::hash(const Socket& sk)
{
    size_t hash_value = 0x12345678;
    hash_value += std::hash<uint32_t>()(sk.src_addr.in6.s6_addr32[0]);
    hash_value += std::hash<uint32_t>()(sk.src_addr.in6.s6_addr32[1]);
    hash_value += std::hash<uint32_t>()(sk.src_addr.in6.s6_addr32[2]);
    hash_value += std::hash<uint32_t>()(sk.src_addr.in6.s6_addr32[3]);
    hash_value += std::hash<uint32_t>()(sk.dst_addr.in6.s6_addr32[0]);
    hash_value += std::hash<uint32_t>()(sk.dst_addr.in6.s6_addr32[1]);
    hash_value += std::hash<uint32_t>()(sk.dst_addr.in6.s6_addr32[2]);
    hash_value += std::hash<uint32_t>()(sk.dst_addr.in6.s6_addr32[3]);
    hash_value += std::hash<uint16_t>()(sk.src_port);
    hash_value += std::hash<uint16_t>()(sk.dst_port);
    hash_value += std::hash<uint8_t>()(sk.l2_protocol.name);
    hash_value += std::hash<uint8_t>()(sk.l3_protocol.name);
    hash_value += std::hash<uint8_t>()(sk.l4_protocol.name);
    hash_value += std::hash<uint8_t>()(sk.l5_protocol.name);
    return hash_value;
}