#include <iostream>
#include <fstream>
#include <stdint.h>
#include <sstream>
#include <vector>
#include <string>
#include "json/json.hpp"
#include "common/primitives.h"
#include "socket/ftrange.h"

// #define RTE_PKTMBUF_HEADROOM 256
#define MBUF_SIZE (2048 + sizeof(struct rte_mbuf) + RTE_PKTMBUF_HEADROOM)
using json = nlohmann::json;

std::string config_file;

dpdk_config_user usrconfig;

json get_json(std::string filename)
{
    std::ifstream config_stream(filename);
    json j;
    // std::cout << config_stream.rdbuf() << std::endl;
    config_stream >> j;
    return j;
}

FTRange ftranges[MAX_PATTERNS];

bool seq_picked[MAX_PATTERNS] = {false, false, false, false};
int seq_idx[MAX_PATTERNS] = {0, 0, 0, 0};


void static_seq_pick(Socket* socket, void* data)
{
    int pattern = (uint64_t) data;
    if(!seq_picked[pattern])
    {
        set_start_ft(socket, ftranges[pattern]);
        seq_picked[pattern] = true;
    }
    else
    {
        increase_ft(socket, ftranges[pattern]);
    }
}

void dynamic_seq_pick(Socket* socket, void* data)
{
    int pattern = (uint64_t) data;
    if(!seq_picked[pattern])
    {
        set_start_ft(socket, ftranges[pattern]);
        seq_picked[pattern] = true;
    }
    else
    {
        set_by_range_idx_ft(socket, ftranges[pattern], seq_idx[pattern]);
        seq_idx[pattern]++;
        if(seq_idx[pattern] >= ftranges[pattern].total_num)
        {
            seq_idx[pattern] = 0;
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <config_file>" << std::endl;
        return -1;
    }

    if(std::string(argv[1]) == "-c"){
        config_file = argv[2];
    }

    json json_config = get_json(config_file);

    usrconfig.lcores.clear();
    for(auto& lcore : json_config["lcores"]){
        usrconfig.lcores.push_back(lcore);
    }

    usrconfig.ports.clear();
    for(auto& port : json_config["ports"].items()){
        usrconfig.ports.push_back(port.key());
        usrconfig.gateway_for_ports.push_back(port.value());
    }

    if(json_config.contains("always_accurate_time"))
        usrconfig.always_accurate_time = json_config["always_accurate_time"];
    if(json_config.contains("use_preset_flowtable_size"))
        usrconfig.use_preset_flowtable_size = json_config["use_preset_flowtable_size"];
    if(json_config.contains("flowtable_init_size"))
        usrconfig.flowtable_init_size = json_config["flowtable_init_size"];
    if(json_config.contains("rss_type"))
        usrconfig.rss_type = json_config["rss_type"];
    if(json_config.contains("rss_auto_type"))
        usrconfig.rss_auto_type = json_config["rss_auto_type"];
    if(json_config.contains("tx_burst_size"))
        usrconfig.tx_burst_size = json_config["tx_burst_size"];
    if(json_config.contains("rx_burst_size"))
        usrconfig.rx_burst_size = json_config["rx_burst_size"];
    
    primitives::set_configs_and_init(usrconfig, argv);
    primitives::set_pattern_num(json_config["patterns"].size());
    int pattern_idx = 0;
    for(auto& json_pattern : json_config["patterns"])
    {
        protocol_config p_config;
        p_config.protocol = json_pattern["pattern_type"];
        if(json_pattern.contains("mode"))
            p_config.mode = json_pattern["mode"];
        if(json_pattern.contains("preset"))
            p_config.preset = json_pattern["preset"];
        if(json_pattern.contains("use_http"))
            p_config.use_http = json_pattern["use_http"];
        if(json_pattern.contains("use_keepalive"))
            p_config.use_keepalive = json_pattern["use_keepalive"];
        if(json_pattern.contains("keepalive_interval"))
            p_config.keepalive_interval = json_pattern["keepalive_interval"];
        if(json_pattern.contains("keepalive_timeout"))
            p_config.keepalive_timeout = json_pattern["keepalive_timeout"];
        if(json_pattern.contains("keepalive_request_maxnum"))
            p_config.keepalive_request_maxnum = json_pattern["keepalive_request_maxnum"];
        if(json_pattern.contains("just_send_first_packet"))
            p_config.just_send_first_packet = json_pattern["just_send_first_packet"];
        if(json_pattern.contains("launch_batch"))
            p_config.launch_batch = json_pattern["launch_batch"];
        if(json_pattern.contains("payload_random"))
            p_config.payload_random = json_pattern["payload_random"];
        if(json_pattern.contains("payload_size"))
            p_config.payload_size = json_pattern["payload_size"];
        if(json_pattern.contains("send_window"))
            p_config.send_window = json_pattern["send_window"];
        if(json_pattern.contains("mss"))
            p_config.mss = json_pattern["mss"];
        if(json_pattern.contains("tos"))
            p_config.tos = json_pattern["tos"];
        if(p_config.protocol == "TCP")
            if(json_pattern.contains("cps"))
                p_config.cps = json_pattern["cps"];
        else
            if(json_pattern.contains("pps"))
                p_config.cps = json_pattern["pps"];
        if(json_pattern.contains("cc"))
            p_config.cc = json_pattern["cc"];
        if(json_pattern.contains("template_ip_src"))
            p_config.template_ip_src = json_pattern["template_ip_src"];
        if(json_pattern.contains("template_ip_dst"))
            p_config.template_ip_dst = json_pattern["template_ip_dst"];
        if(json_pattern.contains("template_port_src"))
            p_config.template_port_src = json_pattern["template_port_src"];
        if(json_pattern.contains("template_port_dst"))
            p_config.template_port_dst = json_pattern["template_port_dst"];

        primitives::add_pattern(p_config);

        if(json_pattern.contains("fivetuples_preset"))
        {
            if(!p_config.preset)
                std::cout << "Error: fivetuples_preset can only be used with preset pattern" << std::endl;
            FiveTuples ths_ft;
            for (auto& fivetuple : json_pattern["fivetuples_preset"])
            {
                ths_ft.src_addr = std::string(fivetuple["src_ip"]);
                ths_ft.dst_addr = std::string(fivetuple["dst_ip"]);
                ths_ft.src_port = std::atoi(std::string(fivetuple["src_port"]).c_str());
                ths_ft.dst_port = std::atoi(std::string(fivetuple["dst_port"]).c_str());
                primitives::add_fivetuples(ths_ft, pattern_idx);
            }
        } 
        else if(json_pattern.contains("fivetuples_range"))
        {
            ftranges[pattern_idx].start_src_ip = std::string(json_pattern["fivetuples_range"]["start_src_ip"]);
            ftranges[pattern_idx].src_ip_num = atoi(std::string(json_pattern["fivetuples_range"]["src_ip_num"]).c_str());
            ftranges[pattern_idx].start_dst_ip = std::string(json_pattern["fivetuples_range"]["start_dst_ip"]);
            ftranges[pattern_idx].dst_ip_num = atoi(std::string(json_pattern["fivetuples_range"]["dst_ip_num"]).c_str());
            ftranges[pattern_idx].start_src_port = atoi(std::string(json_pattern["fivetuples_range"]["start_src_port"]).c_str());
            ftranges[pattern_idx].src_port_num = atoi(std::string(json_pattern["fivetuples_range"]["src_port_num"]).c_str());
            ftranges[pattern_idx].start_dst_port = atoi(std::string(json_pattern["fivetuples_range"]["start_dst_port"]).c_str());
            ftranges[pattern_idx].dst_port_num = atoi(std::string(json_pattern["fivetuples_range"]["dst_port_num"]).c_str());
            init_ft_range(&ftranges[pattern_idx]);
            set_start_ft(&template_socket[pattern_idx], ftranges[pattern_idx]);
            if(p_config.protocol == "TCP")
            {
                if(p_config.preset)
                {
                    if(!json_pattern["fivetuples_range"].contains("use_flowtable") || json_pattern["fivetuples_range"]["use_flowtable"])
                    {
                        int total_num = ftranges[pattern_idx].src_ip_num * ftranges[pattern_idx].dst_ip_num * ftranges[pattern_idx].src_port_num * ftranges[pattern_idx].dst_port_num;
                        if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                        {
                             
                        }
                        else
                        {
                            for(int i=0; i<total_num; i++)
                            {
                                primitives::add_fivetuples(template_socket[pattern_idx], pattern_idx);
                                increase_ft(&template_socket[pattern_idx], ftranges[pattern_idx]);
                            }
                        }
                    }
                    else
                    {

                    }
                }
                else
                {
                    if(!json_pattern["fivetuples_range"].contains("use_flowtable") || json_pattern["fivetuples_range"]["use_flowtable"])
                    {
                        if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                        {

                        }
                        else
                        {
                            if(p_config.mode == "client")
                            {
                                primitives::set_random_method(dynamic_seq_pick, pattern_idx, (void*) pattern_idx);
                            }
                        }
                    }
                    else
                    {

                    }
                }
            }
        }
        pattern_idx++;
    }

    primitives::run_generate();
}