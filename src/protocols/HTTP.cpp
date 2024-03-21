#include "protocols/HTTP.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>


#define HTTP_REQ_FORMAT         \
    "GET %s HTTP/1.1\r\n"       \
    "User-Agent: dperf\r\n"     \
    "Host: %s\r\n"              \
    "Accept: */*\r\n"           \
    "P: aa\r\n"                 \
    "\r\n"

/* don't change */
#define HTTP_RSP_FORMAT         \
    "HTTP/1.1 200 OK\r\n"       \
    "Content-Length:%11d\r\n"  \
    "Connection:keep-alive\r\n" \
    "\r\n"                      \
    "%s"

static char http_rsp[MBUF_DATA_SIZE];
static char http_req[MBUF_DATA_SIZE];
static const char *http_rsp_body_default = "hello dperf!\r\n";

HTTP* parser_http = new HTTP();

int HTTP::payload_size;
bool HTTP::payload_random;
char HTTP::http_host[HTTP_HOST_MAX];
char HTTP::http_path[HTTP_PATH_MAX];
__thread struct HTTP::delay_vec HTTP::ack_delay;

const char *http_get_request(void)
{
    return http_req;
}

const char *http_get_response(void)
{
    return http_rsp;
}
static inline int http_header_match(const uint8_t *start, int name_len, int name_size,
    uint8_t f0, uint8_t f1, uint8_t l0, uint8_t l1)
{
    uint8_t first = 0;
    uint8_t last = 0;

    if (name_len == name_size) {
        first = start[0];
        last = start[name_size - 2];
        /* use the first and the last letter to identify 'content-length'*/
        if (((first == f0) || (first == f1)) && ((last == l0) || (last == l1))) {
            return 1;
        }
    }

    return 0;
}

int http_ack_delay_flush()
{
    int i = 0;
    struct Socket *sk = NULL;

    for (i = 0; i < HTTP::ack_delay.next; i++) {
        sk = HTTP::ack_delay.sockets[i];
        HTTP* http = (HTTP*)sk->l5_protocol;
        TCP* tcp = (TCP*)sk->l4_protocol;
        if (http->http_ack) {
            http->http_ack = 0;
            if (tcp->state == TCP_ESTABLISHED) {
                tcp_reply(tcp, sk, TH_ACK);
            }
       }
    }

    if (HTTP::ack_delay.next > 0) {
        HTTP::ack_delay.next = 0;
    }

    return 0;
}

static inline int http_parse_header_line(struct Socket *sk, const uint8_t *start, int name_len, int line_len)
{
    int i = 0;
    long content_length = 0;
    HTTP* http = (HTTP*)sk->l5_protocol;
    TCP* tcp = (TCP*)sk->l4_protocol;

    if (http_header_match(start, name_len, CONTENT_LENGTH_SIZE, 'C', 'c', 'h', 'H')) {
        content_length = atol((const char *)(start + CONTENT_LENGTH_SIZE));
        if (content_length < 0) {
            return -1;
        }

        if ((http->http_flags & HTTP_F_TRANSFER_ENCODING) == 0) {
            http->http_length = content_length;
            http->http_flags |= HTTP_F_CONTENT_LENGTH;
        } else {
            return -1;
        }
    } else if (http_header_match(start, name_len, TRANSFER_ENCODING_SIZE, 'T', 't', 'g', 'G')) {
        /* find 'chunked' in 'gzip/deflate/chunked' */
        for (i = name_len; i < line_len; i++) {
            /* 'k' stands for 'chunked' */
            if ((start[i] == 'k') || (start[i] == 'K')) {
                if ((http->http_flags & HTTP_F_CONTENT_LENGTH) == 0) {
                    http->http_flags |= HTTP_F_TRANSFER_ENCODING;
                    break;
                } else {
                    return -1;
                }
            }
        }
    } else if (http_header_match(start, name_len, CONNECTION_SIZE, 'C', 'c', 'n', 'N')) {
        if (line_len < (name_len + (int)STRING_SIZE("keep-alive") + 1)) {
            http->http_flags |= HTTP_F_CLOSE;
            tcp->keepalive = 0;
        }
    }

    return 0;
}

/*
 * headers must be in an mbuf
 * */
static int http_parse_headers(struct Socket *sk, const uint8_t *data, int data_len)
{
    HTTP* http = (HTTP*)sk->l5_protocol;
    TCP* tcp = (TCP*)sk->l4_protocol;
    const uint8_t *p = NULL;
    const uint8_t *start = NULL;
    const uint8_t *end = NULL;
    uint8_t c = 0;
    int line_len = 0;
    int name_len = 0;

    start = data;
    p = data;
    end = data + data_len;
    while (p < end) {
        c = *p;
        p++;
        line_len++;
        if ((c == ':') && (name_len == 0)) {
            name_len = line_len;
        } else if (c == '\r') {
            continue;
        } else if (c == '\n') {
            if (http->http_parse_state == HTTP_HEADER_BEGIN) {
                if (http_parse_header_line(sk, start, name_len, line_len) < 0) {
                    return -1;
                }
                line_len = 0;
                name_len = 0;
                start = p;
                http->http_parse_state = HTTP_HEADER_LINE_END;
            } else {
            /* end of header */
                http->http_parse_state = HTTP_HEADER_DONE;
                if (http->http_flags == 0) {
                    http->http_flags = HTTP_F_CONTENT_LENGTH_AUTO | HTTP_F_CLOSE;
                    http->http_length = -1;
                    tcp->keepalive = 0;
                }
                break;
            }
        } else if (http->http_parse_state != HTTP_HEADER_BEGIN) {
            http->http_parse_state = HTTP_HEADER_BEGIN;
        }
    }

    return p - data;
}

/*
 * https://datatracker.ietf.org/doc/html/rfc7230
 * */
static inline int http_parse_chunk(struct Socket *sk, const uint8_t *data, int data_len)
{
    HTTP* http = (HTTP*)sk->l5_protocol;
    TCP* tcp = (TCP*)sk->l4_protocol;

    uint8_t c = 0;
    int len = 0;
    const uint8_t *end = data + data_len;
    const uint8_t *p = data;

retry:
    switch (http->http_parse_state) {
    case HTTP_HEADER_DONE:
        http->http_parse_state = HTTP_CHUNK_SIZE;
        /* fall through */
    case HTTP_CHUNK_SIZE:
        while (p < end) {
            c = *p;
            p++;
            if ((c >= '0') && (c <= '9')) {
                http->http_length = (http->http_length << 4) + c - '0';
            } else if ((c >= 'a') && (c <= 'f')) {
                http->http_length = (http->http_length << 4) + c - 'a' + 10;
            } else {
                http->http_parse_state = HTTP_CHUNK_SIZE_END;
                break;
            }
        }
        /* fall through */
    case HTTP_CHUNK_SIZE_END:
        while (p < end) {
            c = *p;
            p++;
            if (c == '\n') {
                if (http->http_length > 0) {
                    http->http_parse_state = HTTP_CHUNK_DATA;
                    break;
                } else {
                    http->http_parse_state = HTTP_CHUNK_TRAILER_BEGIN;
                    goto trailer_begin;
                }
            }
            /* skip chunk ext */
        }
        /* fall through */
    case HTTP_CHUNK_DATA:
        /* fall through */
        if (p < end) {
            len = end - p;
            if (http->http_length >= len) {
                http->http_length -= len;
                return HTTP_PARSE_OK;
            }

            p += http->http_length;
            http->http_length = 0;
            c = *p;
            p++;
            if (c == '\r') {
                http->http_parse_state = HTTP_CHUNK_DATA_END;
            } else {
                return HTTP_PARSE_ERR;
            }
        } else {
            return HTTP_PARSE_OK;
        }
        /* fall through */
    case HTTP_CHUNK_DATA_END:
        if (p < end) {
            c = *p;
            p++;
            if (c == '\n') {
                http->http_parse_state = HTTP_CHUNK_SIZE;
                goto retry;
            } else {
                return HTTP_PARSE_ERR;
            }
        } else {
            return HTTP_PARSE_OK;
        }
        /* fall through */
    case HTTP_CHUNK_TRAILER_BEGIN:
trailer_begin:
        if (p < end) {
            c = *p;
            p++;
            if (c == '\r') {
                http->http_parse_state = HTTP_CHUNK_END;
                goto chunk_end;
            } else {
                http->http_parse_state = HTTP_CHUNK_TRAILER;
            }
        } else {
            return HTTP_PARSE_OK;
        }
        /* fall through */
    case HTTP_CHUNK_TRAILER:
        while (p < end) {
            c = *p;
            p++;
            if (c == '\n') {
                http->http_parse_state = HTTP_CHUNK_TRAILER_BEGIN;
                goto trailer_begin;
            }
        }
        return HTTP_PARSE_OK;

    case HTTP_CHUNK_END:
chunk_end:
        if (p < end) {
            c = *p;
            p++;
            if (c == '\n') {
                http->http_parse_state = HTTP_BODY_DONE;
                return HTTP_PARSE_END;
            } else {
                return HTTP_PARSE_ERR;
            }
        } else {
            return HTTP_PARSE_OK;
        }
    default:
        return HTTP_PARSE_ERR;
    }
}

static inline int http_parse_body(struct Socket *sk, const uint8_t *data, int data_len)
{
    HTTP* http = (HTTP*)sk->l5_protocol;
    TCP* tcp = (TCP*)sk->l4_protocol;

    if (http->http_flags & HTTP_F_CONTENT_LENGTH) {
        if (data_len < http->http_length) {
            http->http_length -= data_len;
            return HTTP_PARSE_OK;
        } else if (data_len == http->http_length) {
            http->http_length = 0;
            http->http_parse_state = HTTP_BODY_DONE;
            return HTTP_PARSE_END;
        } else {
            return HTTP_PARSE_ERR;
            return -1;
        }
    } else if (http->http_flags & HTTP_F_TRANSFER_ENCODING) {
        return http_parse_chunk(sk, data, data_len);
    } else {
        return HTTP_PARSE_OK;
    }
}

int http_parse_run(struct Socket *sk, const uint8_t *data, int data_len)
{
    HTTP* http = (HTTP*)sk->l5_protocol;
    TCP* tcp = (TCP*)sk->l4_protocol;

    int len = 0;

    if (http->http_parse_state == HTTP_INIT) {
        http_parse_response(data, data_len);
        http->http_parse_state = HTTP_HEADER_BEGIN;
    }

    if (http->http_parse_state < HTTP_HEADER_DONE) {
        len = http_parse_headers(sk, data, data_len);
        if (len < 0) {
            return HTTP_PARSE_ERR;
        }

        data += len;
        data_len -= len;
    }

    if (data_len >= 0) {
        return http_parse_body(sk, data, data_len);
    } else {
        return HTTP_PARSE_OK;
    }
}

void http_set_payload(char *data, int len, int new_line)
{
    int i = 0;
    int num = 'z' - 'a' + 1;
    struct timeval tv;

    if (len == 0) {
        data[0] = 0;
        return;
    }

    if (!HTTP::payload_random) {
        memset(data, 'a', len);
    } else {
        gettimeofday(&tv, NULL);
        srandom(tv.tv_usec);
        for (i = 0; i < len; i++) {
            data[i] = 'a' + random() % num;
        }
    }

    if ((len > 1) && new_line) {
        data[len - 1] = '\n';
    }

    data[len] = 0;
}


static void http_set_payload_client(char *dest, int len, int payload_size)
{
    int pad = 0;
    char buf[MBUF_DATA_SIZE] = {0};

    if (payload_size <= 0) {
        snprintf(dest, len, HTTP_REQ_FORMAT, HTTP::http_path, HTTP::http_host);
    } else if (payload_size < HTTP_DATA_MIN_SIZE) {
        http_set_payload(dest, payload_size, 1);
    } else {
        pad = payload_size - HTTP_DATA_MIN_SIZE;
        if (pad > 0) {
            http_set_payload(buf, pad, 0);
        }
        buf[0] = '/';
        snprintf(dest, len, HTTP_REQ_FORMAT, buf, HTTP::http_host);
    }
}

static void http_set_payload_server(char *dest, int len, int payload_size)
{
    int pad = 0;
    int content_length = 0;
    char buf[MBUF_DATA_SIZE] = {0};
    const char *data = NULL;

    if (payload_size <= 0) {
        data = http_rsp_body_default;
        snprintf(dest, len, HTTP_RSP_FORMAT, (int)strlen(data), data);
    } else if (payload_size < HTTP_DATA_MIN_SIZE) {
        http_set_payload(dest, payload_size, 1);
    } else {
        if (payload_size > TCP::global_mss) {
            pad = TCP::global_mss - HTTP_DATA_MIN_SIZE;
        } else {
            pad = payload_size - HTTP_DATA_MIN_SIZE;
        }

        content_length = payload_size - HTTP_DATA_MIN_SIZE;
        if (pad > 0) {
            http_set_payload(buf, pad, 1);
        }
        snprintf(dest, len, HTTP_RSP_FORMAT, content_length, buf);
    }
}

void http_set_payload(int payload_size)
{
    if (TCP::server) {
        http_set_payload_server(http_rsp, MBUF_DATA_SIZE, payload_size);
    } else {
        http_set_payload_client(http_req, MBUF_DATA_SIZE, payload_size);
    }
}
