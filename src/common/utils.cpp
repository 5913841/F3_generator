#include <vector>
#include <string>
#include <sstream>
#include "common/utils.h"
#include <iostream>
#include <regex>
#include <netinet/ether.h>

std::vector<std::string> split(const std::string& s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}

//A function to convert a ip netaddress from string to integer
unsigned ipv4_netaddress_to_int(std::string ip_netaddress)
{
    unsigned ip_int = 0;
    std::vector<std::string> ip_vec = split(ip_netaddress, '.');
    for(int i = 0; i < 4; i++){
        ip_int = ip_int * 256 + std::stoi(ip_vec[i]);
    }
    return ip_int;
}


//A function to convert a ip netaddress from integer to string
std::string ipv4_netaddress_to_string(unsigned ip_netaddress)
{
    std::string ip_string = "";
    std::vector<std::string> ip_vec;
    for(int i = 0; i < 4; i++){
        ip_vec.push_back(std::to_string(ip_netaddress % 256));
        ip_netaddress = ip_netaddress / 256;
    }
    ip_vec.reserve(4);
    for(int i = 3; i >= 0; i--){
        ip_string += ip_vec[i] + ".";
    }
    return ip_string.substr(0, ip_string.length() - 1);
}

//A function to distinguish ipv6 address ipv4 address and mac address
int distinguish_address_type(std::string address)
{
    if(address.find(':') != std::string::npos){
        return 6;
    }
    else if(address.find('.') != std::string::npos){
        return 4;
    }
    else{
        return 2;
    }
}

std::string distinguish_address_type(const std::string& address) {
    std::regex ipv6Pattern("^([0-9a-fA-F]{1,4}:){7}([0-9a-fA-F]{1,4})$");
    std::regex ipv4Pattern(R"(^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)");
    std::regex macPattern("^([0-9a-fA-F]{2}[:-]){5}([0-9a-fA-F]{2})$");

    if (std::regex_match(address, ipv6Pattern)) {
        return "IPv6";
    } else if (std::regex_match(address, ipv4Pattern)) {
        return "IPv4";
    } else if (std::regex_match(address, macPattern)) {
        return "MAC Address";
    } else {
        return "Unknown";
    }
}

void ether_addr_copy(ether_addr dst, const ether_addr src) {
    memcpy(dst.ether_addr_octet, src.ether_addr_octet, sizeof(ether_addr));
}

void ether_addr_copy(uint8_t* dst, const ether_addr src) {
    memcpy(dst, src.ether_addr_octet, sizeof(ether_addr));
}

void ether_addr_copy(ether_addr dst, const uint8_t* src) {
    memcpy(dst.ether_addr_octet, src, sizeof(ether_addr));
}

void ether_addr_copy(uint8_t* dst, const uint8_t* src) {
    memcpy(dst, src, sizeof(ether_addr));
}

void ether_header_init_addr(ether_header* header, ether_addr shost, ether_addr dhost) {
    ether_addr_copy(header->ether_shost, shost);
    ether_addr_copy(header->ether_dhost, dhost);
}