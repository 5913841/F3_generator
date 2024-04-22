#ifndef HTTP_H
#define HTTP_H

#include "protocols/base_protocol.h"
#include "protocols/TCP.h"
#include "timer/clock.h"
#include "dpdk/dpdk_config.h"

class HTTP;

extern thread_local HTTP *parser_http;

enum HTTP_STATE
{
    HTTP_IDLE,
    HTTP_WAITING_FOR_REQUEST,
    HTTP_WAITING_FOR_RESPONSE,
    HTTP_SENDING_RESPONSE,
    HTTP_DONE
};

enum
{
    HTTP_INIT,
    HTTP_HEADER_BEGIN,
    HTTP_HEADER_LINE_END,
    HTTP_HEADER_DONE,
    HTTP_CHUNK_SIZE,
    HTTP_CHUNK_SIZE_END,
    HTTP_CHUNK_DATA,
    HTTP_CHUNK_DATA_END,
    HTTP_CHUNK_TRAILER_BEGIN,
    HTTP_CHUNK_TRAILER,
    HTTP_CHUNK_END,
    HTTP_BODY_DONE,
    HTTP_ERROR
};

class HTTP : public L5_Protocol
{
public:
    // #ifdef HTTP_PARSE
    /* ------16 bytes------  */
    /* http protocol         */
    int64_t http_length;
    uint8_t http_parse_state;
    uint8_t http_flags;
    uint8_t http_ack;
    uint8_t snd_window;
    uint32_t snd_max;
    HTTP_STATE state;
    static __thread int payload_size;
    static __thread bool payload_random;
    static __thread char http_host[HTTP_HOST_MAX];
    static __thread char http_path[HTTP_PATH_MAX];
    static __thread struct delay_vec
    {
        int next;
        struct Socket *sockets[HTTP_ACK_DELAY_MAX];
    } ack_delay;
    // #endif
    HTTP() : L5_Protocol() {}
    ProtocolCode name() override { return ProtocolCode::PTC_HTTP; }
    inline size_t get_hdr_len(Socket *socket, rte_mbuf *data)
    {
        return 0;
    }
    size_t hash() override
    {
        return std::hash<uint8_t>()(PTC_HTTP);
    }
    inline int construct(Socket *socket, rte_mbuf *data)
    {
        return 0;
    }
    inline void process(Socket *socket, rte_mbuf *data)
    {
    }

    static int parse_packet_ft(rte_mbuf *data, FiveTuples *ft, int offset)
    {
        ft->protocol_codes[SOCKET_L5] = ProtocolCode::PTC_HTTP;
        return 0;
    }

    static int parse_packet_sk(rte_mbuf *data, Socket *sk, int offset)
    {
        sk->l5_protocol = parser_http;
        return 0;
    }

    static void parser_init()
    {
        L5_Protocol::parser.add_parser(parse_packet_ft);
        L5_Protocol::parser.add_parser(parse_packet_sk);
    }
};

static inline void http_parse_response(const uint8_t *data, uint16_t len)
{
    net_stats_tcp_rsp();
    /* HTTP/1.1 200 OK */
    if ((len > 9) && data[9] == '2') {
        net_stats_http_2xx();
    } else {
        net_stats_http_error();
    }
}

static inline void http_parse_request(const uint8_t *data, uint16_t len)
{
    net_stats_tcp_req();
    /*
     * GET /xxx HTTP/1.1
     * First Char is G
     * */
    if ((len > 18) && data[0] == 'G') {
        net_stats_http_get();
    } else {
        net_stats_http_error();
    }
}

#define HTTP_DATA_MIN_SIZE 70
void http_set_payload(int payload_size);
const char *http_get_request(void);
const char *http_get_response(void);

// #ifdef HTTP_PARSE
int http_ack_delay_flush();

static inline void http_ack_delay_add(struct Socket *sk)
{
    HTTP *http = (HTTP *)sk->l5_protocol;
    if (http->http_ack)
    {
        return;
    }

    if (HTTP::ack_delay.next >= HTTP_ACK_DELAY_MAX)
    {
        http_ack_delay_flush();
    }

    HTTP::ack_delay.sockets[HTTP::ack_delay.next] = sk;
    HTTP::ack_delay.next++;
    http->http_ack = 1;
}

// #ifdef HTTP_PARSE
static inline void socket_init_http(struct HTTP *http)
{
    http->http_length = 0;
    http->http_parse_state = 0;
    http->http_flags = 0;
    http->http_ack = 0;
}

static inline void socket_init_http_server(struct HTTP *http, TCP *tcp)
{
    http->http_length = 0;
    http->http_parse_state = 0;
    http->http_flags = 0;
    http->http_ack = 0;
    http->snd_max = tcp->snd_nxt + (uint32_t)http->payload_size;
}

// #else
// #define socket_init_http(http) do{}while(0)
// #define socket_init_http_server(http) do{}while(0)
// #endif

int http_parse_run(struct Socket *sk, const uint8_t *data, int data_len);

static inline uint8_t http_client_process_data(struct Socket *sk,
                                               uint8_t rx_flags, uint8_t *data, uint16_t data_len)
{
    HTTP *http = (HTTP *)sk->l5_protocol;
    TCP *tcp = (TCP *)sk->l4_protocol;
    int ret = 0;
    int8_t tx_flags = 0;

    ret = http_parse_run(sk, data, data_len);
    if (ret == HTTP_PARSE_OK)
    {
        if ((rx_flags & TH_FIN) == 0)
        {
            http_ack_delay_add(sk);
            return 0;
        }
    }
    else if (ret == HTTP_PARSE_END)
    {
        socket_init_http(http);
        if (tcp->keepalive && ((rx_flags & TH_FIN) == 0))
        {
            http_ack_delay_add(sk);
            tcp_start_keepalive_timer(sk, time_in_config());
            return 0;
        }
        else
        {
            tx_flags |= TH_FIN;
            http->http_ack = 0;
        }
    }
    else
    {
        socket_init_http(http);
        tcp->keepalive = 0;
        http->http_length = 0;
        tx_flags |= TH_FIN;
        net_stats_http_error();
    }

    return TH_ACK | tx_flags;
}
// #endif

#endif