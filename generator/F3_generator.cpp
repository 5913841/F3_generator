#include <iostream>
#include <fstream>
#include <stdint.h>
#include <sstream>
#include <vector>
#include <string>
#include <random>
#include "json/json.hpp"
#include "common/primitives.h"
#include "socket/ftrange.h"
#include "api/api.h"

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

int stringToInt(const std::string &str) {
    int num = 0;
    for (char c : str) {
        if (std::isdigit(c)) {
            num = num * 10 + (c - '0');
        } else {
            break;
        }
    }
    return num;
}

int parse_time_to_seconds(const std::string &time_str) {
    // Determine the length of the input string
    int len = time_str.length();
    if (len == 0) return 0;

    // Find the first non-digit character
    int i = 0;
    while (i < len && std::isdigit(time_str[i])) {
        ++i;
    }

    // Extract the numeric part
    int num = stringToInt(time_str);

    // Determine the unit of time
    std::string unit = time_str.substr(i);

    // Convert to seconds based on the unit
    if (unit == "h") {
        return num * 3600;  // hours to seconds
    } else if (unit == "min" || unit == "m") {
        return num * 60;    // minutes to seconds
    } else if (unit == "s" || unit == "") {
        return num;         // seconds
    } else {
        // If the unit is unrecognized, return 0 or handle as needed
        return 0;
    }
}


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

void rand_pick(Socket* socket, void* data)
{
    int pattern = (uint64_t) data;
    FTRange& ftrange = ftranges[pattern];

    socket->dst_addr = ftrange.start_dst_ip + rand_() % ftrange.dst_ip_num;
    socket->src_addr = ftrange.start_src_ip + rand_() % ftrange.src_ip_num;
    socket->src_port = ftrange.start_src_port + rand_() % ftrange.src_port_num;
    socket->dst_port = ftrange.start_dst_port + rand_() % ftrange.dst_port_num;
}

struct ScanningData
{
    int pattern;
    ip4addr_t base_dst_ip;
    int min_cseg_num_per_round;
    int cseg_choice_num_per_round;
    int time_per_cseg;
    int start_second;
    bool preset;
    bool pick_random;
    int preset_idx;

    ip4addr_t* ip_c_segs;
    int ip_c_seg_num;
    int ths_c_seg_idx;
};

void scanning_ft_pick(Socket* socket, void* data)
{
    ScanningData* scd = (ScanningData*) data;
    int start_second = scd->start_second;
    int cseg_choice_num_per_round = scd->cseg_choice_num_per_round;
    int time_per_cseg = scd->time_per_cseg;
    int min_cseg_num_per_round = scd->min_cseg_num_per_round;
    ip4addr_t base_dst_ip = scd->base_dst_ip;
    int now_second = api::api_get_time_second();
    if(start_second == -1 || now_second >= start_second + time_per_cseg)
    {
        scd->start_second = now_second;
        scd->ip_c_seg_num = rand_() % cseg_choice_num_per_round + min_cseg_num_per_round;
        for (int i = 0; i < scd->ip_c_seg_num; i++)
        {
            scd->ip_c_segs[i] = base_dst_ip + ((rand_() % 256) << 8);
        }
        scd->ths_c_seg_idx = 0;
    }
    if(scd->preset)
    {
        socket->src_addr = primitives::socket_partby_pattern[primitives::get_index(scd->pattern,scd->preset_idx)].src_addr;
        socket->src_port = primitives::socket_partby_pattern[primitives::get_index(scd->pattern,scd->preset_idx)].src_port;
        socket->dst_port = primitives::socket_partby_pattern[primitives::get_index(scd->pattern,scd->preset_idx)].dst_port;
        scd->preset_idx++;
        if(scd->preset_idx >= primitives::socketsize_partby_pattern[scd->pattern])
        {
            scd->preset_idx = 0;
        }
    }
    else
    {
        if(scd->pick_random)
        {
            rand_pick(socket, (void*)scd->pattern);
        }
        else
        {
            dynamic_seq_pick(socket, (void*)scd->pattern);
        }
    }
    socket->dst_addr = scd->ip_c_segs[scd->ths_c_seg_idx] + rand_() % 256;
    scd->ths_c_seg_idx++;
    if(scd->ths_c_seg_idx == scd->ip_c_seg_num)
    {
        scd->ths_c_seg_idx = 0;
    }
}

struct PulseData
{
    int pattern;
    uint64_t max_speed_interval;

    tsc_time start_tsc;
    tsc_time end_tsc;
};

void init_pulse_data(PulseData* pd, int pattern, int max_pps, int launch_num, uint64_t attack_interval, uint64_t duration_each_time, uint64_t start_time)
{
    api::api_time_update();
    pd->pattern = pattern;
    pd->max_speed_interval = (g_tsc_per_second * launch_num) / max_pps;

    pd->start_tsc.last = time_in_config() + start_time - attack_interval;
    pd->end_tsc.last = time_in_config() + start_time + duration_each_time - attack_interval;
    pd->start_tsc.interval = attack_interval;
    pd->end_tsc.interval = attack_interval;
    pd->start_tsc.count = 0;
    pd->end_tsc.count = 0;
}

void update_speed_pulse(launch_control* lc, void* data)
{
    PulseData* pd = (PulseData*) data;
    if(unlikely(time_in_config() < pd->start_tsc.last))
    {
        lc->launch_interval = g_tsc_per_second * (1 << 14);
        return;
    }
    if(unlikely(tsc_time_go(&pd->start_tsc, time_in_config())))
    {
        lc->launch_last = time_in_config();
        lc->launch_interval = pd->max_speed_interval;
    }
    if(unlikely(tsc_time_go(&pd->end_tsc, time_in_config())))
    {
        lc->launch_interval = g_tsc_per_second * (1 << 14);
    }
}

ScanningData scanning_data[MAX_PATTERNS];
PulseData pulse_data[MAX_PATTERNS];


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
        if(json_pattern.contains("slow_start"))
            p_config.slow_start = json_pattern["slow_start"];
        if(json_pattern.contains("template_ip_src"))
            p_config.template_ip_src = json_pattern["template_ip_src"];
        if(json_pattern.contains("template_ip_dst"))
            p_config.template_ip_dst = json_pattern["template_ip_dst"];
        if(json_pattern.contains("template_port_src"))
            p_config.template_port_src = json_pattern["template_port_src"];
        if(json_pattern.contains("template_port_dst"))
            p_config.template_port_dst = json_pattern["template_port_dst"];

        primitives::add_pattern(p_config);

        if(json_pattern.contains("gen_mode") && json_pattern["gen_mode"] == "scanning")
        {
            ScanningData& scd = scanning_data[pattern_idx];
            scd.pattern = pattern_idx;
            scd.base_dst_ip = std::string(json_pattern["base_dst_ip"]);
            scd.min_cseg_num_per_round = json_pattern["min_cseg_num_per_round"];
            scd.cseg_choice_num_per_round = (int)json_pattern["max_cseg_num_per_round"] - (int)json_pattern["min_cseg_num_per_round"];
            scd.ip_c_segs = new ip4addr_t[(int)json_pattern["max_cseg_num_per_round"]];
            scd.time_per_cseg = parse_time_to_seconds(std::string(json_pattern["time_per_cseg"]));
            scd.start_second = -1;
            scd.preset = p_config.preset;
            scd.preset_idx = 0;
            primitives::set_random_method(scanning_ft_pick, pattern_idx, (void*) &scanning_data[pattern_idx]);
        }
        else if(json_pattern.contains("gen_mode") && json_pattern["gen_mode"] == "pulse")
        {
            PulseData& pd = pulse_data[pattern_idx];

            init_pulse_data(&pd, pattern_idx, config_parse_number(std::string(json_pattern["max_pps"]).c_str(), true, true), config_parse_number(p_config.launch_batch.c_str(), false, false),
                                                config_parse_time(std::string(json_pattern["attack_interval"]).c_str()),
                                                config_parse_time(std::string(json_pattern["duration_each_time"]).c_str()),
                                                config_parse_time(std::string(json_pattern["start_time"]).c_str()));
            primitives::set_update_speed_method(update_speed_pulse, pattern_idx, (void*) &pulse_data[pattern_idx]);
        }

        if(json_pattern.contains("fivetuples_preset"))
        {
            if(!p_config.preset)
                std::cout << "Error: fivetuples_preset can only be used with preset pattern" << std::endl;
            FiveTuples ths_ft;
            if(json_pattern.contains("gen_mode") && json_pattern["gen_mode"] == "scanning")
            {
                for (auto& fivetuple : json_pattern["fivetuples_preset"])
                {
                    ths_ft.src_addr = std::string(fivetuple["src_ip"]);
                    ths_ft.dst_addr = 0;
                    ths_ft.src_port = std::atoi(std::string(fivetuple["src_port"]).c_str());
                    ths_ft.dst_port = std::atoi(std::string(fivetuple["dst_port"]).c_str());
                    primitives::add_fivetuples(ths_ft, pattern_idx);
                }
            }
            else{
                for (auto& fivetuple : json_pattern["fivetuples_preset"])
                {
                    ths_ft.src_addr = std::string(fivetuple["src_ip"]);
                    ths_ft.dst_addr = std::string(fivetuple["dst_ip"]);
                    ths_ft.src_port = std::atoi(std::string(fivetuple["src_port"]).c_str());
                    ths_ft.dst_port = std::atoi(std::string(fivetuple["dst_port"]).c_str());
                    primitives::add_fivetuples(ths_ft, pattern_idx);
                }
            }
        }
        else if(json_pattern.contains("fivetuples_range"))
        {
            if(json_pattern.contains("gen_mode") && json_pattern["gen_mode"] == "scanning")
            {
                ftranges[pattern_idx].start_src_ip = std::string(json_pattern["fivetuples_range"]["start_src_ip"]);
                ftranges[pattern_idx].src_ip_num = atoi(std::string(json_pattern["fivetuples_range"]["src_ip_num"]).c_str());
                ftranges[pattern_idx].start_src_port = atoi(std::string(json_pattern["fivetuples_range"]["start_src_port"]).c_str());
                ftranges[pattern_idx].start_dst_ip = 0;
                ftranges[pattern_idx].dst_ip_num = 1;
                ftranges[pattern_idx].src_port_num = atoi(std::string(json_pattern["fivetuples_range"]["src_port_num"]).c_str());
                ftranges[pattern_idx].start_dst_port = atoi(std::string(json_pattern["fivetuples_range"]["start_dst_port"]).c_str());
                ftranges[pattern_idx].dst_port_num = atoi(std::string(json_pattern["fivetuples_range"]["dst_port_num"]).c_str());
                init_ft_range(&ftranges[pattern_idx]);
                set_start_ft(&template_socket[pattern_idx], ftranges[pattern_idx]);
                int total_num = ftranges[pattern_idx].src_ip_num * ftranges[pattern_idx].src_port_num * ftranges[pattern_idx].dst_port_num;
                if(p_config.preset){
                    if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                    {                        std::vector<int> idxs;
                        for(int i=0; i<total_num; i++)
                        {
                            idxs.push_back(i);
                        }
                        std::shuffle(idxs.begin(), idxs.end(), std::mt19937(std::random_device{}()));
                        for(int i=0; i<total_num; i++)
                        {
                            set_by_range_idx_ft(&template_socket[pattern_idx], ftranges[pattern_idx], idxs[i]);
                            primitives::add_fivetuples(template_socket[pattern_idx], pattern_idx);
                        }

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
                    if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                        scanning_data[pattern_idx].pick_random = true;
                    else
                        scanning_data[pattern_idx].pick_random = true;
                }
            }
            else
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
                if(p_config.preset)
                {
                    if(!json_pattern["fivetuples_range"].contains("use_flowtable") || json_pattern["fivetuples_range"]["use_flowtable"])
                    {
                        int total_num = ftranges[pattern_idx].total_num;
                        if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                        {
                            std::vector<int> idxs;
                            for(int i=0; i<total_num; i++)
                            {
                                idxs.push_back(i);
                            }
                            std::shuffle(idxs.begin(), idxs.end(), std::mt19937(std::random_device{}()));
                            for(int i=0; i<total_num; i++)
                            {
                                set_by_range_idx_ft(&template_socket[pattern_idx], ftranges[pattern_idx], idxs[i]);
                                primitives::add_fivetuples(template_socket[pattern_idx], pattern_idx);
                            }
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
                        p_config.ft_range = ftranges[pattern_idx];
                        int total_num = ftranges[pattern_idx].total_num;
                        for(int i=0; i<total_num; i++)
                        {
                            primitives::add_fivetuples(template_socket[pattern_idx], pattern_idx);
                            increase_ft(&template_socket[pattern_idx], ftranges[pattern_idx]);
                        }
                    }
                }
                else
                {
                    if(!json_pattern["fivetuples_range"].contains("use_flowtable") || json_pattern["fivetuples_range"]["use_flowtable"])
                    {
                        if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                        {
                            if(p_config.mode == "client")
                            {
                                primitives::set_random_method(rand_pick, pattern_idx, (void*) pattern_idx);
                            }
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
                        p_config.ft_range = ftranges[pattern_idx];
                        if(json_pattern["fivetuples_range"].contains("pick_random") && json_pattern["fivetuples_range"]["pick_random"])
                        {
                            if(p_config.mode == "client")
                            {
                                primitives::set_random_method(rand_pick, pattern_idx, (void*) pattern_idx);
                            }
                        }
                        else
                        {
                            if(p_config.mode == "client")
                            {
                                primitives::set_random_method(dynamic_seq_pick, pattern_idx, (void*) pattern_idx);
                            }
                        }
                    }
                }
            }
        }
        pattern_idx++;
    }

    primitives::run_generate();
}