#include "ripv2.h"
#include "udp.h"
#include "ipv4.h"
#include "ipv4_route_table.h"
#include "ipv4_config.h"
#include "arp.h"
#include "eth.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <arpa/inet.h>
#include <timerms.h>
#include <time.h>


int main ( int argc, char * argv[] ){

    if ( argc != 4 ) {
        printf("        <string>: Archivo de configuración del Cliente\n");
        printf("        <string>: Archivo de tabla de rutas del Cliente\n");
        printf("         <ipv4>: Dirección IP del servidor RIP\n");
        exit(-1);
    }

    char * config_file = argv[1];
    char * route_file = argv[2];
    char * server_ip_str = argv[3];

    ipv4_addr_t server_ip;

    int err = ipv4_str_addr(server_ip_str, server_ip);
    if (err < 0) {
        printf("ERROR en ipv4_str_addr(\"%s\")\n", server_ip_str);
        exit(-1);
    }
    
    //Abrimos layer UDP
    udp_layer_t * layer = udp_open(config_file, route_file, 0);
    if (layer == NULL) {
        printf("ERROR en udp_open()\n");
        exit(-1);
    }


    ripv2_frame_t frame;
    ripv2_route_t request_route; //RFC RIP para hacer un request

    request_route.family = htons(2);
    request_route.tag = 0;
    request_route.metric = htonl(16);
    memcpy(request_route.ipv4_address, IPv4_ZERO_ADDR, 4);
    memcpy(request_route.ipv4_mask, IPv4_ZERO_ADDR, 4);
    memcpy(request_route.next_hop, IPv4_ZERO_ADDR, 4);
    
    frame.type = 1;
    frame.version = 2;
    frame.routing_domain = 0;
    frame.routes[0] = request_route;
    uint16_t ripv2_port = 520;

    int sent_bytes = udp_send(layer, server_ip, ripv2_port, (unsigned char *)&frame, //(int)sizeof(ripv2_frame_t));
24);

//int udp_send(udp_layer_t * layer, ipv4_addr_t dst_ip, uint16_t dst_port, unsigned char * payload, int payload_len);
    if (sent_bytes <= 0) {
        printf("ERROR en udp_send()\n");
        exit(-1);
    }

    printf("Recibiendo tramas UDP\n");

    unsigned char buffer[ETH_MTU - IPv4_HEADER_SIZE - UDP_HEADER_SIZE];
    ripv2_frame_t frame_recv;


    int recv_bytes = udp_recv(layer, server_ip, &ripv2_port, buffer, sizeof(ripv2_frame_t), 20000);
    if (recv_bytes < 1) {
        printf("ERROR en udp_recv()\n");
        exit(-1);
    }


    memcpy(&frame_recv, &buffer, sizeof(ripv2_frame_t));

    for(int i = 0; i<25; i++){

        ripv2_route_print(&frame_recv.routes[i]);

    }

    err = udp_close(layer);
    if(err < 0){

        printf("Error closing udp layer in ripv2_client");
        exit(-1);

    }

    return 0;

    //configurar escenario simple con router y probarlo
}








