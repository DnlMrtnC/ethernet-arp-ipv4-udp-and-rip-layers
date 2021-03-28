#ifndef _ARP_H
#define _ARP_H

#include <stdint.h>
#include "eth.h"
#include "ipv4.h"

#define ARP_MTU 1480 //ETH_MTU - ARP_HEADER

typedef struct arp_frame arp_frame_t;

int arp_resolve(eth_iface_t *iface, ipv4_addr_t src_ip, ipv4_addr_t ip_addr, mac_addr_t mac_addr);

#endif /* _ARP_H */
