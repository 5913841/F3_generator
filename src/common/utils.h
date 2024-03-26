#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <netinet/ether.h>

// A function to convert a ip netaddress from string to integer
unsigned ipv4_netaddress_to_int(std::string ip_netaddress);

// A function to convert a ip netmask from string to integer
int ip_netmask_to_int(std::string ip_netmask);

// A function to convert a ip netaddress from integer to string
std::string ipv4_netaddress_to_string(unsigned ip_netaddress);

std::string distinguish_address_type(const std::string &address);

void ether_addr_copy(ether_addr *dst, const ether_addr *src);

void ether_addr_copy(uint8_t *dst, const ether_addr src);

void ether_addr_copy(ether_addr dst, const uint8_t *src);

void ether_addr_copy(uint8_t *dst, const uint8_t *src);

void ether_header_init_addr(ether_header *header, ether_addr shost, ether_addr dhost);

#endif /* UTILS_H */