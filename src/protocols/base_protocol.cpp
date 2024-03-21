#include "base_protocol.h"
#include "socket/socket.h"

PacketParser L2_Protocol::parser = {.parser_fivetuples_num = 0, .parser_socket_num = 0};
PacketParser L3_Protocol::parser = {.parser_fivetuples_num = 0, .parser_socket_num = 0};
PacketParser L4_Protocol::parser = {.parser_fivetuples_num = 0, .parser_socket_num = 0};
PacketParser L5_Protocol::parser = {.parser_fivetuples_num = 0, .parser_socket_num = 0};




int PacketParser::parse_packet(rte_mbuf* data, FiveTuples* ft, int offset){
    for(int i = 0; i < parser_fivetuples_num; i++)
    {
        parse_fivetuples_t parse_func = parser_fivetuples[i];
        int res = parse_func(data, ft, offset);
        if(res != -1)
        {
            return res;
        }
    }
    return -1;
    
}

int PacketParser::parse_packet(rte_mbuf* data, Socket* sk, int offset){
    for(int i = 0; i < parser_socket_num; i++)
    {
        parse_socket_t parse_func = parser_socket[i];
        int res = parse_func(data, sk, offset);
        if(res != -1)
        {
            return res;
        }
    }
    return -1;
    
}

void PacketParser::add_parser(parse_fivetuples_t parse_func){
    if(likely(parser_fivetuples_num < MAX_PROTOCOLS_PER_LAYER))
    {
        parser_fivetuples[parser_fivetuples_num] = parse_func;
        parser_fivetuples_num++;
    }
    else
    {
        printf("PacketParser: Too many protocols per layer\n");
    }
}

void PacketParser::add_parser(parse_socket_t parse_func){
    if(likely(parser_socket_num < MAX_PROTOCOLS_PER_LAYER))
    {
        parser_socket[parser_socket_num] = parse_func;
        parser_socket_num++;
    }
    else
    {
        printf("PacketParser: Too many protocols per layer\n");
    }
}

int parse_packet(rte_mbuf* data, FiveTuples* ft) {
    int offset = 0;
    int res = L2_Protocol::parser.parse_packet(data, ft, offset);
    if (unlikely(res < 0)) {
        printf("L2 parse failed\n");
    } else {
        offset += res;
    }

    res = L3_Protocol::parser.parse_packet(data, ft, offset);
    if (unlikely(res < 0)) {
        printf("L3 parse failed\n");
    } else {
        offset += res;
    }

    res = L4_Protocol::parser.parse_packet(data, ft, offset);
    if (unlikely(res < 0)) {
        printf("L4 parse failed\n");
    } else {
        offset += res;
    }

    res = L5_Protocol::parser.parse_packet(data, ft, offset);
    if (unlikely(res < 0)) {
        printf("L5 parse failed\n");
    } else {
        offset += res;
    }
     
    return offset;
}

int parse_packet(rte_mbuf* data, Socket* sk) {
    int offset = 0;
    int res = L2_Protocol::parser.parse_packet(data, sk, offset);
    if (unlikely(res < 0)) {
        printf("L2 parse failed\n");
    } else {
        offset += res;
    }

    res = L3_Protocol::parser.parse_packet(data, sk, offset);
    if (unlikely(res < 0)) {
        printf("L3 parse failed\n");
    } else {
        offset += res;
    }

    res = L4_Protocol::parser.parse_packet(data, sk, offset);
    if (unlikely(res < 0)) {
        printf("L4 parse failed\n");
    } else {
        offset += res;
    }

    res = L5_Protocol::parser.parse_packet(data, sk, offset);
    if (unlikely(res < 0)) {
        printf("L5 parse failed\n");
    } else {
        offset += res;
    }
     
    return offset;
}