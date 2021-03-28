#include "udp.h"
#include "ipv4.h"
#include "arp.h"
#include "eth.h"

#include <timerms.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

typedef struct udp_layer{
    ipv4_layer_t * ipv4_layer;
    uint16_t src_port;
} udp_layer_t;



typedef struct udp_frame{
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t frame_len;
    uint16_t checksum;
    uint8_t payload[ETH_MTU - IPv4_HEADER_SIZE - UDP_HEADER_SIZE];
} udp_frame_t;


udp_layer_t * udp_open(char * file_conf, char * conf_route, int port){

    ipv4_layer_t * ipv4_layer = ipv4_open(file_conf, conf_route);
    if(ipv4_layer == NULL){
        printf("Error al abrir la interface ip");
        exit(-1);
    }

    int src_port;
    if(port == 0)
        src_port = gen_random_number();
    else
        src_port = port;

    udp_layer_t * udp_layer = malloc(sizeof(udp_layer_t));

    udp_layer->ipv4_layer = ipv4_layer;
    udp_layer->src_port = src_port;

    return udp_layer;

}


int udp_send(udp_layer_t * layer, ipv4_addr_t dst_ip, uint16_t dst_port, unsigned char * payload, int payload_len){

    udp_frame_t frame;
    uint16_t src_port = layer->src_port;
    frame.src_port = htons(src_port);
    frame.dst_port = htons(dst_port);
    frame.frame_len = htons(payload_len + UDP_HEADER_SIZE);
    frame.checksum = htons(0);
    memcpy(frame.payload, payload, payload_len);

    //pasamos dst_ip desde el cliente/server udp.
    short udp_protocol = 17;
    int err = ipv4_send (layer->ipv4_layer, dst_ip, udp_protocol, (unsigned char *)&frame, UDP_HEADER_SIZE + payload_len);
    if(err < 1){
        printf("Error en udp_send al llamar a ipv4_send\n");
        exit(-1);
    }

    return err;

}



int udp_recv(udp_layer_t * layer, ipv4_addr_t src_addr, uint16_t * src_port, unsigned char buffer[], int buf_len, long int timeout){

    udp_frame_t frame_recv;
    ipv4_addr_t sender_ip;
    uint16_t protocol = 17; //protocolo UDP
    unsigned char ipv4_payload[ETH_MTU - IPv4_HEADER_SIZE];
    int ipv4_payload_len = ETH_MTU - IPv4_HEADER_SIZE;
    
    int recv_bytes = 0;

    do{

        recv_bytes = ipv4_recv(layer->ipv4_layer, protocol, ipv4_payload, sender_ip, ipv4_payload_len, timeout);
        printf("recv bytes(udp): %d\n", recv_bytes);
        memcpy(&frame_recv, &ipv4_payload, ipv4_payload_len);

    }while(ntohs(frame_recv.dst_port) != *src_port );
/*
    if (timeout == -1){
        while(1){
            recv_bytes = ipv4_recv(layer->ipv4_layer, protocol, ipv4_payload, sender_ip, ipv4_payload_len, 5000);
            memcpy(&frame_recv, &ipv4_payload, ipv4_payload_len);
            if(ntohs(frame_recv.dst_port) == layer->src_port){
                break;
            }
        }
    } else{

        while(time_left > 1){
            recv_bytes = ipv4_recv(layer->ipv4_layer, protocol, ipv4_payload, sender_ip, ipv4_payload_len, timeout);
            printf("recv bytes(udp): %d\n", recv_bytes);
            memcpy(&frame_recv, &ipv4_payload, ipv4_payload_len);
            if(ntohs(frame_recv.dst_port) == layer->src_port){
                break;
            }
            time_left = timerms_left(&timer);
        }
    }
*/

    //int frame_len = frame_recv.frame_len;
    //payload_len = frame_len - UDP_HEADER_SIZE;
    if (buf_len > recv_bytes) {//ponia payload_len
        buf_len = recv_bytes;  //ponia payload _len
    }
    memcpy(buffer, frame_recv.payload, buf_len);

    memcpy(src_addr, sender_ip, IPv4_ADDR_SIZE);
    *src_port = ntohs(frame_recv.src_port);
    
    return recv_bytes;

}



int udp_close(udp_layer_t * layer){

    int err = ipv4_close(layer->ipv4_layer);
    if(err == -1){
        printf("Error al cerrar la interfaz ip en udp_close\n");
    }
    free(layer);

    return 1;
}





int gen_random_number(){

    /* Inicializar semilla para rand() */
    unsigned int seed = time(NULL);
    srand(seed);
    /* Generar número aleatorio entre 0 y RAND_MAX */
    int dice = rand();
    /* Número entero aleatorio entre 1 y 10 */
    dice = 1024 + (int) (1000.0 * dice / (RAND_MAX));
    return dice;
}
