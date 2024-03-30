// #include "protocols/UDP.h"
// #include "protocols/TCP.h"
// #include "protocols/IP.h"
// #include "dpdk/dpdk_config.h"
// #include "dpdk/mbuf.h"
// #include "dpdk/mbuf_template.h"
// #include "panel/panel.h"
// #include "socket/socket.h"
// #include "timer/timer.h"
// #include "dpdk/csum.h"
// #include <sys/time.h>
// #include <rte_mbuf.h>

// static char g_udp_data[MBUF_DATA_SIZE] = "hello THUGEN!!\n";
// void udp_set_payload(char *data, int len, int new_line)
// {
//     int i = 0;
//     int num = 'z' - 'a' + 1;
//     struct timeval tv;

//     if (len == 0)
//     {
//         data[0] = 0;
//         return;
//     }

//     if (!UDP::payload_random)
//     {
//         memset(data, 'a', len);
//     }
//     else
//     {
//         gettimeofday(&tv, NULL);
//         srandom(tv.tv_usec);
//         for (i = 0; i < len; i++)
//         {
//             data[i] = 'a' + random() % num;
//         }
//     }

//     if ((len > 1) && new_line)
//     {
//         data[len - 1] = '\n';
//     }

//     data[len] = 0;
// }

// void udp_set_payload(int page_size)
// {
//     udp_set_payload(g_udp_data, page_size, 1);
// }


// static inline struct rte_mbuf *udp_new_packet(struct Socket *sk)
// {
//     struct rte_mbuf *m = NULL;
//     struct iphdr *iph = NULL;
//     struct udphdr *uh = NULL;
//     struct ip6_hdr *ip6h = NULL;

//     UDP* udp = (UDP*)sk->l4_protocol;
    
//     mbuf_cache* p = udp->template_udp_pkt;

//     m = mbuf_cache_alloc(udp->template_udp_pkt);
//     if (unlikely(m == NULL)) {
//         return NULL;
//     }


//     iph = rte_pktmbuf_mtod_offset(m, struct iphdr *, p->data.l2_len);
//     ip6h = (struct ip6_hdr *)iph;
//     uh = rte_pktmbuf_mtod_offset(m, struct udphdr *, p->data.l2_len + p->data.l3_len);

//     // if (!ipv6) {
//     iph->id = htons(ip_id++);
//     iph->saddr = sk->src_addr;
//     iph->daddr = sk->dst_addr;
//     // } else {
//     //     ip6h->ip6_src.s6_addr32[3] = sk->laddr;
//     //     ip6h->ip6_dst.s6_addr32[3] = sk->faddr;
//     // }

//     uh->source = sk->src_port;
//     uh->dest = sk->dst_port;
//     uh->check = csum_pseudo_ipv4(IPPROTO_UDP, sk->src_addr, sk->dst_addr, p->data.l4_len);

//     return m;
// }

// static inline struct rte_mbuf* udp_send(struct Socket *sk)
// {
//     struct rte_mbuf *m = NULL;

//     m = udp_new_packet(sk);
//     if (m) {
//         dpdk_config_percore::cfg_send_packet(m);
//     }

//     return m;
// }

// static void udp_close(struct Socket *sk)
// {
//     UDP* udp = (UDP*)sk->l4_protocol;
//     udp->state = TCP_CLOSE;
//     UDP::release_socket_callback(sk);

// }

// static inline void udp_retransmit_handler(struct Socket *sk)
// {
//     /* rss auto: this socket is closed by another worker */
//     if (sk->src_addr != 0) {
//         net_stats_udp_rt();
//     }

//     UDP::release_socket_callback(sk);
// }

// static inline void udp_send_request(UDP* udp, struct Socket *sk)
// {
//     udp->state = TCP_SYN_SENT;
//     udp->keepalive_request_num++;
//     udp_send(sk);
// }

// inline bool udp_in_duration()
// {
//     if ((current_ts_msec() < (uint64_t)(UDP::global_duration_time)) && (UDP::global_stop == false))
//     {
//         return true;
//     }
//     else
//     {
//         return false;
//     }
// }

// void tcp_start_keepalive_timer(TCP *tcp, struct Socket *sk, uint64_t now_tsc)
// {
//     if (tcp->keepalive && (tcp->snd_nxt == tcp->snd_una))
//     {
//         tcp->timer_tsc = now_tsc;
//         TIMERS.add_job(new KeepAliveTimer(sk, now_tsc), now_tsc);
//     }
// }

// static void udp_socket_keepalive_timer_handler(struct Socket *sk)
// {
//     int pipeline = UDP::pipeline;
//     UDP* udp = (UDP*)sk->l4_protocol;

//     if (udp_in_duration()) {
//         /* rss auto: this socket is closed by another worker */
//         if (unlikely(sk->src_addr == 0)) {
//             udp_close(sk);
//             return;
//         }

//         /* if not flood mode, let's check packet loss */
//         if ((!UDP::flood) && (udp->state == TCP_SYN_RECV)) {
//             net_stats_udp_rt();
//         }

//         do {
//             udp_send_request(udp, sk);
//             pipeline--;
//         } while (pipeline > 0);
//         if (UDP::keepalive_request_interval) {
//             udp_start_keepalive_timer(sk);
//         }
//     }
// }

// static void udp_client_process(struct work_space *ws, struct rte_mbuf *m)
// {
//     struct iphdr *iph = mbuf_ip_hdr(m);
//     struct tcphdr *th = mbuf_tcp_hdr(m);
//     struct socket *sk = NULL;

//     sk = socket_client_lookup(&ws->socket_table, iph, th);
//     if (unlikely(sk == NULL)) {
//         if (ws->kni && work_space_is_local_addr(ws, m)) {
//             return kni_recv(ws, m);
//         }
//         goto out;
//     }

//     if (sk->state != SK_CLOSED) {
//         sk->state = SK_SYN_RECEIVED;
//     } else {
//         goto out;
//     }
//     if (sk->keepalive == 0) {
//         net_stats_rtt(ws, sk);
//         socket_close(sk);
//     } else if ((g_config.keepalive_request_interval == 0) && (!ws->stop)) {
//         udp_send_request(ws, sk);
//     }

//     mbuf_free(m);
//     return;

// out:
//     net_stats_udp_drop();
//     mbuf_free(m);
// }

// static void udp_server_process(struct work_space *ws, struct rte_mbuf *m)
// {
//     struct iphdr *iph = mbuf_ip_hdr(m);
//     struct tcphdr *th = mbuf_tcp_hdr(m);
//     struct socket *sk = NULL;

//     sk = socket_server_lookup(&ws->socket_table, iph, th);
//     if (unlikely(sk == NULL)) {
//         if (ws->kni && work_space_is_local_addr(ws, m)) {
//             return kni_recv(ws, m);
//         }
//         goto out;
//     }

//     udp_send(ws, sk);
//     mbuf_free(m);
//     return;

// out:
//     net_stats_udp_drop();
//     mbuf_free(m);
// }

// static int udp_client_launch(struct work_space *ws)
// {
//     uint64_t i = 0;
//     struct socket *sk = NULL;
//     uint64_t num = 0;
//     int pipeline = g_config.pipeline;

//     num = work_space_client_launch_num(ws);
//     for (i = 0; i < num; i++) {
//         sk = socket_client_open(&ws->socket_table, work_space_tsc(ws));
//         if (unlikely(sk == NULL)) {
//             continue;
//         }

//         do {
//             udp_send(ws, sk);
//             pipeline--;
//         } while (pipeline > 0);
//         if (sk->keepalive) {
//             if (g_config.keepalive_request_interval) {
//                 /* for rtt calculationn */
//                 socket_start_keepalive_timer(sk, work_space_tsc(ws));
//             }
//         } else if (ws->flood) {
//             socket_close(sk);
//         } else {
//             socket_start_retransmit_timer_force(sk, work_space_tsc(ws));
//         }
//     }

//     return num;
// }

// static inline int udp_client_socket_timer_process(struct work_space *ws)
// {
//     struct socket_timer *rt_timer = &g_retransmit_timer;
//     struct socket_timer *kp_timer = &g_keepalive_timer;

//     if (g_config.keepalive) {
//         socket_timer_run(ws, kp_timer, g_config.keepalive_request_interval, udp_socket_keepalive_timer_handler);
//     } else {
//         socket_timer_run(ws, rt_timer, RETRANSMIT_TIMEOUT, udp_retransmit_handler);
//     }
//     return 0;
// }

// static inline int udp_server_socket_timer_process(__rte_unused struct work_space *ws)
// {
//     return 0;
// }

// int udp_init(struct work_space *ws)
// {
//     if (g_config.protocol != IPPROTO_UDP) {
//         return 0;
//     }

//     if (g_config.server) {
//         if (ws->vxlan) {
//             ws->run_loop = udp_server_run_loop_vxlan;
//         } else if (ws->ipv6) {
//             ws->run_loop = udp_server_run_loop_ipv6;
//         } else {
//             ws->run_loop = udp_server_run_loop_ipv4;
//         }
//     } else {
//         if (ws->vxlan) {
//             ws->run_loop = udp_client_run_loop_vxlan;
//         } else if (ws->ipv6) {
//             ws->run_loop = udp_client_run_loop_ipv6;
//         } else {
//             ws->run_loop = udp_client_run_loop_ipv4;
//         }
//     }

//     return mbuf_cache_init_udp(&ws->udp, ws, "udp", g_udp_data);
// }

// void udp_drop(__rte_unused struct work_space *ws, struct rte_mbuf *m)
// {
//     if (m) {
//         if (ws->kni && work_space_is_local_addr(ws, m)) {
//             return kni_recv(ws, m);
//         }
//         MBUF_LOG(m, "drop");
//         net_stats_udp_drop();
//         mbuf_free2(m);
//     }
// }
