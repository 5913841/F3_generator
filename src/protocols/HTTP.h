#ifndef HTTP_H
#define HTTP_H

#include "protocols/TCP.h"
#include "timer/clock.h"
#include "dpdk/dpdk_config.h"

class HTTP;

extern thread_local HTTP *parser_http;

struct global_http_vars
{
    int payload_size;
    bool payload_random;
    char http_host[HTTP_HOST_MAX];
    char http_path[HTTP_PATH_MAX];
    struct delay_vec
    {
        int next;
        struct Socket *sockets[HTTP_ACK_DELAY_MAX];
    } ack_delay;
};

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

class HTTP
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
    static int pattern_num;
    static __thread global_http_vars g_vars[MAX_PATTERNS];
    
    // #endif
    inline int construct(Socket *socket, rte_mbuf *data)
    {
        return 0;
    }
    inline void process(Socket *socket, rte_mbuf *data)
    {
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
void http_set_payload(int payload_size, int pattern);
const char *http_get_request(int pattern);
const char *http_get_response(int pattern);

// #ifdef HTTP_PARSE
int http_ack_delay_flush();

void http_ack_delay_add(struct Socket *sk);

// #ifdef HTTP_PARSE
static inline void socket_init_http(struct HTTP *http)
{
    http->http_length = 0;
    http->http_parse_state = 0;
    http->http_flags = 0;
    http->http_ack = 0;
}

static inline void socket_init_http_server(struct HTTP *http, TCP *tcp, int pattern)
{
    http->http_length = 0;
    http->http_parse_state = 0;
    http->http_flags = 0;
    http->http_ack = 0;
    http->snd_max = tcp->snd_nxt + (uint32_t)HTTP::g_vars[pattern].payload_size;
}

// #else
// #define socket_init_http(http) do{}while(0)
// #define socket_init_http_server(http) do{}while(0)
// #endif

int http_parse_run(struct Socket *sk, const uint8_t *data, int data_len);

uint8_t http_client_process_data(struct Socket *sk,
                                               uint8_t rx_flags, uint8_t *data, uint16_t data_len);
// #endif

#endif