#include "protocols/conf_protocol.h"

thread_local struct ETHER *ether = new ETHER();
thread_local struct IPV4 ip[MAX_PATTERNS];
thread_local struct Socket template_socket[MAX_PATTERNS];

thread_local mbuf_cache template_tcp_data[MAX_PATTERNS];
thread_local mbuf_cache template_tcp_opt[MAX_PATTERNS];
thread_local mbuf_cache template_tcp_pkt[MAX_PATTERNS];
thread_local char const* data[MAX_PATTERNS];

char *config_str_find_nondigit(char *s, bool float_enable)
{
    char *p = s;
    int point = 0;

    while (*p) {
        if ((*p >= '0') && (*p <= '9')) {
            p++;
            continue;
        } else if (float_enable && (*p == '.')){
            p++;
            point++;
            if (point > 1) {
                return NULL;
            }
        } else {
            return p;
        }
    }

    return NULL;
}

int config_parse_number(const char *str, bool float_enable, bool rate_enable)
{
    char *p = NULL;
    int rate = 1;
    int val = 0;


    char str_[1000];
    memset(str_, 0, 1000);
    memcpy(str_, str, strlen(str));
    p = config_str_find_nondigit(str_, float_enable);
    if (p != NULL) {
        if (rate_enable == false) {
            return -1;
        }

        if (strlen(p) != 1) {
            return -1;
        }

        if ((*p == 'k') || (*p == 'K')) {
            rate = 1000;
        } else if ((*p == 'm') || (*p == 'M')) {
            rate = 1000000;
        } else {
            return -1;
        }
    }

    if (p == str) {
        return -1;
    }

    if (float_enable) {
        val = atof(str) * rate;
    } else {
        val = atoi(str) * rate;
    }

    if (val < 0) {
        return -1;
    }

    return val;
}

uint64_t config_parse_time(const char *str)
{
    char *p = NULL;
    uint64_t val = 0;

    char str_[1000];
    memset(str_, 0, 1000);
    memcpy(str_, str, strlen(str));
    p = config_str_find_nondigit(str_, false);
    if (p == NULL) {
        return -1;
    }

    val = atoi(str);
    if (val < 0) {
        return -1;
    }


    if (strcmp(p, "us") == 0) {
        return val * (g_tsc_per_second/1000/1000);
    } else if (strcmp(p, "ms") == 0) {
        return val * (g_tsc_per_second/1000);
    } else if (strcmp(p, "s") == 0) {
        return val * g_tsc_per_second;
    } else if (strcmp(p, "m") == 0) {
        return val * g_tsc_per_second * 60;
    } else{
        return -1;
    }
}

void config_protocols(int pattern, protocol_config *protocol_cfg)
{
    ether->port = g_config_percore->port;
    TCP::g_vars[pattern].flood = protocol_cfg->just_send_first_packet;
    if(protocol_cfg->protocol == "TCP"){
        TCP::g_vars[pattern].global_keepalive = protocol_cfg->use_keepalive;
        if(TCP::g_vars[pattern].global_keepalive)
        {
            TCP::g_vars[pattern].keepalive_request_interval = config_parse_time(protocol_cfg->keepalive_interval.data());
            TCP::g_vars[pattern].setted_keepalive_request_num = atoi(protocol_cfg->keepalive_request_maxnum.data());
            if(TCP::g_vars[pattern].setted_keepalive_request_num == 0 && protocol_cfg->keepalive_timeout != "0s")
            {
                TCP::g_vars[pattern].setted_keepalive_request_num = round(double(config_parse_time(protocol_cfg->keepalive_timeout.data())) / TCP::g_vars[pattern].keepalive_request_interval);
            }
        }

        if (protocol_cfg->mode == "server")
        {
            TCP::g_vars[pattern].server = true;
        }
        else if (protocol_cfg->mode == "client")
        {
            TCP::g_vars[pattern].server = false;
        }
        else
        {
            printf("Invalid mode for TCP, using default mode (client)");
            TCP::g_vars[pattern].server = false;
        }
        int cc = config_parse_number(protocol_cfg->cc.c_str(), true, true);
        int cps = config_parse_number(protocol_cfg->cps.c_str(), true, true);
        TCP::g_vars[pattern].tos = atoi(protocol_cfg->tos.data());
        TCP::g_vars[pattern].template_tcp_data = &template_tcp_data[pattern];
        TCP::g_vars[pattern].template_tcp_opt = &template_tcp_opt[pattern];
        TCP::g_vars[pattern].template_tcp_pkt = &template_tcp_pkt[pattern];
        TCP::g_vars[pattern].preset = protocol_cfg->preset;
        TCP::g_vars[pattern].use_http = protocol_cfg->use_http;

        TCP* template_tcp = &template_socket[pattern].tcp;
        data[pattern] = TCP::g_vars[pattern].server ? http_get_response(pattern) : http_get_request(pattern);
        template_tcp->retrans = 0;
        template_tcp->keepalive_request_num = 0;
        template_tcp->keepalive = TCP::g_vars[0].global_keepalive;
        uint32_t seed = (uint32_t)rte_rdtsc();
        template_tcp->snd_nxt = rand_r(&seed);
        template_tcp->snd_una = template_tcp->snd_nxt;
        template_tcp->rcv_nxt = 0;
        template_tcp->state = TCP_CLOSE;

        HTTP::g_vars[pattern].payload_size = atoi(protocol_cfg->payload_size.data());
        HTTP::g_vars[0].payload_random = protocol_cfg->payload_random;
        strcpy(HTTP::g_vars[pattern].http_host, HTTP_HOST_DEFAULT);
        strcpy(HTTP::g_vars[pattern].http_path, HTTP_PATH_DEFAULT);
        http_set_payload(HTTP::g_vars[0].payload_size, 0);
        HTTP* template_http = &template_socket[pattern].http;
        template_http->http_length = 0;
        template_http->http_parse_state = 0;
        template_http->http_flags = 0;
        template_http->http_ack = 0;

        template_socket->protocol = IPPROTO_TCP;
        template_socket->pattern = pattern;
        template_socket->src_addr = ip4addr_t(protocol_cfg->template_ip_src);
        template_socket->dst_addr = ip4addr_t(protocol_cfg->template_ip_dst);
        template_socket->src_port = atoi(protocol_cfg->template_port_src.data());
        template_socket->dst_port = atoi(protocol_cfg->template_port_dst.data());

        launch_control_init(&g_config_percore->launch_ctls[pattern], cps, cc, atoi(protocol_cfg->launch_batch.data()));
        constructor constructors[4];
        constructors[0] = [](Socket *sk, rte_mbuf *m){return ether->construct(sk, m);};
        constructors[1] = [pattern](Socket *sk, rte_mbuf *m){return (ip+pattern)->construct(sk, m);};
        constructors[2] = [template_tcp](Socket *sk, rte_mbuf *m){return template_tcp->construct(sk, m);};
        constructors[3] = [template_http](Socket *sk, rte_mbuf *m){return template_http->construct(sk, m);};
        template_tcp_data->mbuf_pool = mbuf_pool_create(g_config, (std::string("template_tcp_data") + "_" + std::to_string(g_config_percore->lcore_id) + "_" + (std::to_string(pattern))).c_str(), g_config_percore->port_id, g_config_percore->queue_id);
        mbuf_template_pool_setby_constructors(template_tcp_data, template_socket, constructors, data[pattern], strlen(data[pattern]));
        template_tcp_pkt->mbuf_pool = mbuf_pool_create(g_config, (std::string("template_tcp_pkt") + "_" + std::to_string(g_config_percore->lcore_id) + "_" + (std::to_string(pattern))).c_str(), g_config_percore->port_id, g_config_percore->queue_id);
        mbuf_template_pool_setby_constructors(template_tcp_pkt, template_socket, constructors, nullptr, 0);
        TCP::g_vars[pattern].constructing_opt_tmeplate = true;
        template_tcp_opt->mbuf_pool = mbuf_pool_create(g_config, (std::string("template_tcp_opt") + "+" + std::to_string(g_config_percore->lcore_id) + "_" + (std::to_string(pattern))).c_str(), g_config_percore->port_id, g_config_percore->queue_id);
        mbuf_template_pool_setby_constructors(template_tcp_opt, template_socket, constructors, nullptr, 0);

        TCP::tcp_init(pattern);
    }
}