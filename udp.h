#ifndef _UDP_H
#define _UDP_H

#include <stdint.h>

#define UDP_HEADER_SIZE 8
#define IPv4_ADDR_SIZE 4

typedef unsigned char ipv4_addr_t [IPv4_ADDR_SIZE];

typedef struct udp_layer udp_layer_t;

udp_layer_t * udp_open(char * file_conf, char * conf_route, int port);

int udp_send(udp_layer_t * layer, ipv4_addr_t dst_ip, uint16_t dst_port, unsigned char * payload, int payload_len);

int udp_recv(udp_layer_t * layer, ipv4_addr_t src_addr, uint16_t * src_port, unsigned char buffer[], int buf_len, long int timeout);

int udp_close(udp_layer_t * layer);

int gen_random_number();

#endif /* _UDP_H */
