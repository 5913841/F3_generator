#ifndef TCP_TIMER_H
#define TCP_TIMER_H

#include "timer/timer.h"
#include "protocols/TCP.h"
#include "socket/socket.h"



template <typename Socket>
struct KeepAliveTimer : public Timer<KeepAliveTimer<Socket>> {
    Socket *sk;
    delay = TCP::keepalive_request_interval;
    KeepAliveTimer(Socket *sk) : sk(sk) {}
    int callback() override
    {
        TCP* tcp = (TCP*)sk->l4_protocol;
        return tcp_do_keepalive<Socket>(tcp, sk, current_ts_msec());
    }

};

template <typename Socket>
struct TimeoutTimer : public Timer<TimeoutTimer<Socket>> {
    Socket *sk;
    delay = (RETRANSMIT_TIMEOUT * RETRANSMIT_NUM_MAX) + TCP::keepalive_request_interval;
    TimeoutTimer(Socket *sk) : sk(sk) {}
    int callback() override
    {
        TCP* tcp = (TCP*)sk->l4_protocol;
        return tcp_do_timeout<Socket>(tcp, sk, current_ts_msec());
    }
};

template <typename Socket>
struct RetransmitTimer : public Timer<RetransmitTimer<Socket>> {
    Socket *sk;
    delay = RETRANSMIT_TIMEOUT;
    RetransmitTimer(Socket *sk) : sk(sk) {}
    int callback() override
    {
        TCP* tcp = (TCP*)sk->l4_protocol;
        return tcp_do_retransmit<Socket>(tcp, sk, current_ts_msec());
    }
};


#endif