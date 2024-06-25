# The F3_generator

## overview

This repository aims to provide an excellent **test traffic generator** implementations with the following characteristics.

* Forge all five-tuples.
* Flexible behavior configuration.
* Freely traffic mixing.

## Installation

### Build DPDK

DPDK environment should be build before using, we recommand dpdk-21.11 on which the generator be tested. The dpdk environment can be download in https://core.dpdk.org/download/. And the process of installation is listed in https://doc.dpdk.org/guides/linux_gsg/.

First, uncompress the archive and move to the uncompressed DPDK source directory:

```sh
tar xJf dpdk-<version>.tar.xz
cd dpdk-<version>
```

To configure a DPDK build use:

```sh
meson setup <options> build
```

Once configured, to build and then install DPDK system-wide use:

```sh
cd build
ninja
meson install
ldconfig
```

Then, the NIC should be bind to dpdk driver

To bind device `eth1`,``04:00.1``, to the `vfio-pci` driver:

```sh
sudo modprobe vfio-pci
./usertools/dpdk-devbind.py --bind=vfio-pci 04:00.1
```

uio_pci_generic or Mellanox dirver can also be used according to the type of your NIC

Last, huge pages should be validated.

```sh
echo 1024 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
```

### Build F3_generator

You can just use make to build the F3_generator

```sh
cd F3_generator
git submodel init
git submodel update
make
```

## User's Guide

We provide three method to control the process of the traffic generation.

### api

We encapsulate the fine-grained functions needed in the process of traffic generation into api functions as follows:

```c++
  socket_set_t& api_get_flowtable(); // return the flowtable.

  uint8_t api_get_syn_pattern_by_res(rte_mbuf *mbuf); // return the syn pattern by res field in tcp header, directly pick the res field from the packet as the pattern.

  const uint8_t& api_get_packet_l4_protocol(rte_mbuf *m); // return the l4 protocol of the packet, either IPPROTO_TCP or IPPROTO_UDP.

  bool api_socket_thscore(Socket *sk); // return true if the socket is in the RSS table.
  
  void api_free_mbuf(rte_mbuf* mbuf); // free the mbuf.

  Socket* api_tcp_new_socket(Socket *temp_sk); // create a new socket same as the temp_sk and return it.

  void api_tcp_validate_socket(Socket *sk); // validate the socket. include the checksum, state, timer.

  void api_tcp_reply(struct Socket *sk, uint8_t tcp_flags); // send a tcp reply packet to the opposite side of the socket according to the socket state and tcp_flags.

  void api_tcp_release_socket(Socket *socket); // release(delete) the socket.

  void api_tcp_launch(Socket *socket); // launch the socket, include valadate the socket and send a tcp syn packet.

  int api_process_tcp(Socket *socket, rte_mbuf *mbuf); // process the mbuf packet according to the socket state by running a simplified tcp state machine.

  void api_tcp_validate_csum(Socket* scoket); // validate the checksum of the socket.

  void api_tcp_validate_csum_opt(Socket* scoket); // just validate the checksum of the option packets of the socket.

  void api_tcp_validate_csum_pkt(Socket *socket); // just validate the checksum of the packet of the socket.

  void api_tcp_validate_csum_data(Socket *socket); // just validate the checksum of the data packets of the socket.

  void api_udp_set_payload(int page_size, int pattern_id); // set the payload of the packet of pattern_id th udp pattern, the payload size is page_size.

  void api_udp_send(Socket *socket); // send a udp packet to the opposite side of the socket.

  char *api_udp_get_payload(int pattern_id); // return the payload of the packet of pattern_id th udp pattern.

  void api_udp_validate_csum(Socket *socket); // validate the checksum of the udp packet of the socket.

  void api_enter_epoch(); // things to do when entering epoch, include updating the time, print the speed.

  int api_check_timer(int pattern); // check how many sockets should be launch in the pattern, return the number of sockets to be launched.
  
  rte_mbuf* api_recv_packet(); // receive a packet from the dpdk NIC.

  Socket* api_parse_packet(rte_mbuf *mbuf); // parse the packet and return the five-tuples of the packet, there are just five-tuples valid in the returned packet.
  
  Socket* api_tcp_new_socket(Socket *temp_sk); // create a new socket same as the temp_sk and return it.
  
  void api_send_packet(rte_mbuf *mbuf); // send a packet to the dpdk NIC.

  void api_send_flush(); // flush the packets to send in the dpdk NIC.

  void api_time_update(); // update the time of the global timer, should be called when you need accurate time (be called in api_enter_epoch() so at least update a time per epoch).

  Socket* api_flowtable_find_socket(Socket* ft_sk); // find the socket in the flowtable by the five-tuples of the socket.

  int api_flowtable_insert_socket(Socket* sk); // insert the socket into the flowtable.

  int api_flowtable_remove_socket(Socket* sk); // remove socket by the five-tuples of the socket provided from the flowtable.

  int api_flowtable_get_socket_count(); // return the number of sockets in the flowtable.

  Socket* api_range_find_socket(Socket* sk, int pattern); // find the socket in the range table by the five-tuples of the socket.

  int api_range_insert_socket(Socket*& sk, int pattern); // change the arg pointer to the socket pointer in the table whose five-tuples is the same as the socket provided and set valid the socket in table. just simulate the insert operation.

  int api_range_remove_socket(Socket* sk, int pattern); // remove(set invalid) the socket from the range table by the five-tuples of the socket provided.

  Socket* api_ptr_range_find_socket(Socket* sk, int pattern); // find the socket in the pointer range table by the five-tuples of the socket.

  int api_ptr_range_insert_socket(Socket* sk, int pattern); // insert the socket into the pointer range table.

  int api_ptr_range_remove_socket(Socket* sk, int pattern); // remove the socket from the pointer range table by the five-tuples of the socket provided.

  Socket* api_tcp_find_socket(Socket* sk, int pattern); // find the socket by the five-tuples of the socket, according to the type of the pattern.

  int api_tcp_insert_socket(Socket*& sk, int pattern); // insert the socket into the table, according to the type of the pattern.

  int api_tcp_remove_socket(Socket* sk, int pattern); // remove the socket from the table, according to the type of the pattern.

  int api_http_ack_delay_flush(int pattern); // flush the http ack delay queue of the pattern.

  void api_trigger_timers(); // trigger all the tcp timers.

  void api_dpdk_run(int (*lcore_main)(void*), void* data); // run the dpdk main function.

  void api_set_configs_and_init(dpdk_config_user& usrconfig, char** argv); // set the dpdk configurations and initialize the dpdk according to the user configuration.

  void api_set_pattern_num(int pattern_num); // set the total number of patterns.

  void api_config_protocols(int pattern, protocol_config *protocol_cfg); // configure and initialize the pattern th protocol of the pattern, according to the protocol_cfg provided.
  
  uint64_t& api_get_time_tsc(); // return the current time in tsc.
  
  int api_get_time_second(); // return the current time in second.
```

### primitives

We encapsulate the coarse-grained functions into primitives to make it more convenient to create a traffic generation script.

```c++
void set_configs_and_init(dpdk_config_user& usrconfig, char** argv); // Use dpdk config data structure to init the dpdk environment.

void set_pattern_num(int pattern_num); // Set the total pattern number used.

void add_pattern(protocol_config& p_config); // Add a manner pattern defined by protocol config data structure.

//should be use in preset mode
void add_fivetuples(const FiveTuples& five_tuples, int pattern); // Set a five-tuples use a specific pattern to generte flow.

void add_fivetuples(const Socket& socket, int pattern); // Set a five-tuples use a specific pattern to generte flow.

//should be use in non-preset mode
void set_random_method(random_socket_t random_method, int pattern_num, void* data); // Set specific manner pattern use a specific method to  generate five-tuples

void set_update_speed_method(update_speed_t update_speed_method, int pattern_num, void* data); // Set specific manner pattern use a specific method to control the speed.

void set_total_time(std::string total_time); // Set the total time the traffic sustained.

void run_generate(); // Start generation.
```

The detail of the used struct is as follows:

* dpdk_config_user

```c++
struct dpdk_config_user
{
    std::vector<int> lcores = {0}; // vector of used lcores' id.
    std::vector<std::string> ports; // vector of used ports' pci name.
    std::vector<std::string> gateway_for_ports; // mac address of gateway for each port.
    std::vector<int> queue_num_per_port = {1}; // number of queues for each port. one cpu core for each queue
    // for example, if lcores = {0, 1, 2, 3}, queue_num_per_port = {3, 1}, then port 0 uses lcores 0, 1, 2, and port 1 uses lcores 3.
    std::string socket_mem = ""; // total hugepage memory for sockets.
    bool always_accurate_time = false; // whether to always use accurate time. if true, update clock when checked every time.
    bool use_preset_flowtable_size = true; // whether to use preset flowtable size.
    std::string flowtable_init_size = "0"; // if use preset flowtable size, initial flowtable size.
    std::string flow_distribution_strategy = "rss"; // Strategy of flow distribution, now we just use rss
    std::string rss_type = "l3"; // Choose the type of rss from l3, l3l4 or auto.
    std::string rss_auto_type = ""; // If use rss auto, choose l3 or l3l4 auto mode to use.
    bool mq_rx_rss = true; // Use ’RTE ETH MQ RX RSS’ to enable the rss on the NIC.
    bool mq_rx_none = false; // Do not use ’RTE ETH MQ RX RSS’ to enable the rss.
    int tx_burst_size = 64; // Number of packets sent in one time in NIC.
    int rx_burst_size = 64; // Number of packts reveived in one time in NIC.
};
```

* protocol_config : the value behind the "=" operator is default value

```c++
struct protocol_config {
    std::string protocol = "TCP"; // The l4 protocol we choose to use, default TCP.
    std::string mode = "client"; // Choose client or server to begin the task.
    bool preset = false; // Whether to preset five-tuples and its pattern by add five-tuples. If not, user should set random function in client.
    bool use_flowtable = true; // Whether to use hash flowtable to store the five-tuples.
    bool use_http = false; // Whether to run the simplified http protocol on TCP.
    bool use_keepalive = false; // Whether to turn on TCP keepalive.
    std::string keepalive_interval = "1ms"; // The interval of client sending keepalive packet.
    std::string keepalive_timeout = "0s"; // If maxnum not set, use time out time to ascertain it.
    std::string keepalive_request_maxnum = "0"; // The max number for client to send keepalive packet.
    std::string launch_batch = "4"; // Flows launched together in one epoch.
    bool payload_random = false; // Randomize the payload of the packet.
    std::string payload_size = "80"; // Set the size of the payload of the packet sent.
    std::string send_window = "0"; // The send window of the TCP protocol.
    std::string mss = std::to_string(MSS_IPV4); // The max segment size of the protocol.
    std::string cps = "1m"; // To control the rate, set connections launched per second.
    std::string tos = "0"; // To control the rate, set maximum connections to launch.
    std::string cc = "0"; // Set the tos field of the packet.
    int slow_start = 0; // Seconds from task begin to the speed increase to the max.
    std::string template_ip_src = "192.168.1.1"; // The source address of template socket under this pattern.
    std::string template_ip_dst = "192.168.1.2"; // The destination address of template socket under this pattern.
    std::string template_port_src = "80"; // The source port of template socket under this pattern.
    std::string template_port_dst = "80"; // The destination port of template socket under this pattern.
    FTRange ft_range; // When not use the flow table, set ft range to control the five-tuples.
};
```

### config file

We use json config file defining the traffic generating manner. An example is as follows.

```json
{
    "lcores" : [0, 1, 2, 3, 4, 5, 6, 7],
    "ports" : {
        "0000:01:00.0" : "98:99:9a:9b:9c:9d",
        "0000:01:00.1" : "98:99:9a:9b:9c:9e",
        "0000:01:00.2" : "98:99:9a:9b:9c:9f"
    },
    "always_accurate_time" : false,
    "use_preset_flowtable_size" : true,
    "flowtable_init_size" : "1M",
    "rss_type" : "l3",
    "rss_auto_type" : "",
    "tx_burst_size" :64,
    "rx_burst_size" :64,
    // 原则上如果有无状态pattern，第一个必须放无状态pattern，否则程序会自动将first_pattern与第一个无状态pattern换位
    // 根据换位后的结果，client程序会根据preset的最大index设置flow_vector的大小为(max_idx_of_preset(increase from 1)-min_idx_of_preset(increase_from_0)) * MAX_SOCKETS_RANGE_PER_PATTERN(1,000,000)，因此preset的pattern最好放在一起
    "patterns" : [
        {
            "pattern_type" : "TCP",
            "mode" : "client",
            "preset": false,
            "use_http": false,
            "use_keepalive": false,
            "keepalive_interval": "1ms",
            "keepalive_timeout": "0s",
            "keepalive_request_maxnum": "0",
            "just_send_first_packet": false,
            "launch_batch": "4",
            "payload_random": false,
            "payload_size": "80",
            "send_window": "0",
            "mss": "1460",
            "tos": "0",
            "cps": "1m",
            "cc": "0",
            "slow_start": 5,
            "template_ip_src": "192.168.1.1",
            "template_ip_dst": "192.168.1.2",
            "template_port_src": "80",
            "template_port_dst": "80",

            "fivetuples_range" : {
                "start_src_ip" : "192.168.1.1",
                "src_ip_num" : "100",

                "start_dst_ip" : "192.168.1.2",
                "dst_ip_num" : "100",

                "start_src_port" : "80",
                "src_port_num" : "100",

                "start_dst_port" : "80",
                "dst_port_num" : "100",

                "pick_random" : false,
                "use_flowtable" : true
            },
						or,
            "fivetuples_preset" : [
                {
                    "src_ip" : "192.168.1.1",
                    "dst_ip" : "192.168.1.2",
                    "src_port" : "80",
                    "dst_port" : "80"
                },
                {
                    "src_ip" : "192.168.1.2",
                    "dst_ip" : "192.168.1.1",
                    "src_port" : "80",
                    "dst_port" : "80"
                }
            ]
        },
        {
            "pattern_type" : "UDP",
            "preset": false,
            "launch_batch": "4",
            "payload_random": false,
            "payload_size": "80",
            "pps": "1m",
            "fivetuples_range" : {
                "start_src_ip" : "192.168.1.1",
                "src_ip_num" : "100",

                "start_dst_ip" : "192.168.1.2",
                "dst_ip_num" : "100",

                "start_src_port" : "80",
                "src_port_num" : "100",

                "start_dst_port" : "80",
                "dst_port_num" : "100",

                "pick_random" : false
            },

            "fivetuples_preset" : [
                {
                    "src_ip" : "192.168.1.1",
                    "dst_ip" : "192.168.1.2",
                    "src_port" : "80",
                    "dst_port" : "80"
                },
                {
                    "src_ip" : "192.168.1.2",
                    "dst_ip" : "192.168.1.1",
                    "src_port" : "80",
                    "dst_port" : "80"
                }
            ]
        },
        {
            "pattern_type" : "TCP_SYN",
            "preset": false,
            "launch_batch": "4",
            "payload_random": false,
            "payload_size": "80",
            "pps": "1m",
            "fivetuples_range" : {
                "start_src_ip" : "192.168.1.1",
                "src_ip_num" : "100",

                "start_dst_ip" : "192.168.1.2",
                "dst_ip_num" : "100",

                "start_src_port" : "80",
                "src_port_num" : "100",

                "start_dst_port" : "80",
                "dst_port_num" : "100",

                "pick_random" : false
            },

            "fivetuples_preset" : [
                {
                    "src_ip" : "192.168.1.1",
                    "dst_ip" : "192.168.1.2",
                    "src_port" : "80",
                    "dst_port" : "80"
                },
                {
                    "src_ip" : "192.168.1.2",
                    "dst_ip" : "192.168.1.1",
                    "src_port" : "80",
                    "dst_port" : "80"
                }
            ]

        },
        {
            "pattern_type" : "TCP_ACK",
            "preset": false,
            "launch_batch": "4",
            "payload_random": false,
            "payload_size": "80",
            "pps": "1m",
            "fivetuples_range" : {
                "start_src_ip" : "192.168.1.1",
                "src_ip_num" : "100",

                "start_dst_ip" : "192.168.1.2",
                "dst_ip_num" : "100",

                "start_src_port" : "80",
                "src_port_num" : "100",

                "start_dst_port" : "80",
                "dst_port_num" : "100",

                "pick_random" : false
            },

            "fivetuples_preset" : [
                {
                    "src_ip" : "192.168.1.1",
                    "dst_ip" : "192.168.1.2",
                    "src_port" : "80",
                    "dst_port" : "80"
                },
                {
                    "src_ip" : "192.168.1.2",
                    "dst_ip" : "192.168.1.1",
                    "src_port" : "80",
                    "dst_port" : "80"
                }
            ]
        },
        {
            "pattern_type" : "TCP_ACK",
            "gen_mode" : "scanning", // "default", "scanning", "pulse"
            "preset": false,
            "launch_batch": "4",
            "payload_random": false,
            "payload_size": "80",
            "pps": "1m",
            "base_dst_ip" : "192.168.0.0",
            "min_cseg_num_per_round" : 6,
            "max_cseg_num_per_round" : 12,
            "time_per_cseg" : "4min",
            "fivetuples_range" : {
                "start_src_ip" : "192.168.1.1",
                "src_ip_num" : "100",
                // no dst_ip
                "start_src_port" : "80",
                "src_port_num" : "100",

                "start_dst_port" : "80",
                "dst_port_num" : "100",

                "pick_random" : false

            },


            "fivetuples_preset" : [
                {   
                    "src_ip" : "192.168.1.1",
                    // no dst_ip
                    "src_port" : "80",
                    "dst_port" : "80"
                },
                {   
                    "src_ip" : "192.168.1.2",
                    "src_port" : "80",
                    "dst_port" : "80"
                }
            ]
        },
        {
            "pattern_type" : "TCP_ACK",
            "gen_mode" : "pulse", // "default", "scanning", "pulse"
            "preset": false,
            "launch_batch": "4",
            "payload_random": false,
            "payload_size": "80",
            "max_pps": "1m",
            "duration_each_time": "10s",
            "attack_interval": "10s",
            "start_time": "0s",
            "fivetuples_range" : {
                "start_src_ip" : "192.168.1.1",
                "src_ip_num" : "100",

                "start_dst_ip" : "192.168.1.2",
                "dst_ip_num" : "100",

                "start_src_port" : "80",
                "src_port_num" : "100",

                "start_dst_port" : "80",
                "dst_port_num" : "100",

                "pick_random" : false
            },

            "fivetuples_preset" : [
                {
                    "src_ip" : "192.168.1.1",
                    "dst_ip" : "192.168.1.2",
                    "src_port" : "80",
                    "dst_port" : "80"
                },
                {
                    "src_ip" : "192.168.1.2",
                    "dst_ip" : "192.168.1.1",
                    "src_port" : "80",
                    "dst_port" : "80"
                }
            ]
        },
    ]

}

```