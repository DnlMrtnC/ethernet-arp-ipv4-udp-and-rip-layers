#include "arp.h"
#include "eth.h"
#include "ipv4.h"
#include <rawnet.h>
#include <timerms.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

/* Tamaño de la cabecera ARP (sin incluir el campo FCS) */
#define ARP_FRAME_SIZE 28

struct arp_frame{
    uint16_t hw_addr_space; //16bit
    uint16_t prot_addr_space;
    unsigned char len_hw_addr; //8bit
    unsigned char len_prot_addr;
    uint16_t opcode;
    mac_addr_t src_mac_addr; //nuestra mac 6byte
    ipv4_addr_t src_ipv4_addr;//nuestra ip 4byte
    mac_addr_t dest_mac_addr;//mac de destino (todo ceros)
    ipv4_addr_t dest_ipv4_addr;//ip de destino
};

/*
int arp_resolve(eth_iface_t *iface, ipv4_addr_t ip_addr, mac_addr_t mac_addr)
 * DESCRIPCIÓN:
 *   Función que nos devuelve la MAC de la IP que proporcionemos
 *
 * PARÁMETROS:
 *   'iface': Manejador de nuestra interfaz Ethernet 
 *            La interfaz debe haber sido inicializada con 'eth_open()'
 *            previamente.
 *    'ip_addr': Manejador de nuestra direccion ip
 *    'mac_addr': Parámetro de salida de la dirección MAC solicitada
 * VALOR DEVUELTO:
 *   1 si encuentra la MAC solicitada. 0 si no.
 * 
 * ERRORES:
 *   La función devuelve '-1' si se ha producido algún error. 
 */

int arp_resolve(eth_iface_t *iface, ipv4_addr_t src_ip, ipv4_addr_t ip_addr, mac_addr_t mac_addr){
    //definir arp frame y rellenar todos los campos
    mac_addr_t unknown_mac = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    struct arp_frame arp_frame;

    //usamos htons en todos los campos numéricos de 16bits (uint16_t)

    //memcpy(arp_frame.hw_addr_space , htons(1), 2);//protocolo ip
    arp_frame.hw_addr_space = htons(1); //protocolo ethernet
    //memcpy(arp_frame.prot_addr_space , htons(0x0800), 2);//protocolo ip
    arp_frame.prot_addr_space = htons(0x0800); //protocolo ip
    //memcpy(arp_frame.len_hw_addr , (unsigned char)MAC_ADDR_SIZE, 1);
    arp_frame.len_hw_addr = MAC_ADDR_SIZE;
    //memcpy(arp_frame.len_prot_addr , (unsigned char)IPv4_ADDR_SIZE, 1);
    arp_frame.len_prot_addr = IPv4_ADDR_SIZE;
    //memcpy(arp_frame.opcode , 1, 2);//request
    mac_addr_t src_mac;
    eth_getaddr(iface, src_mac);
    
    arp_frame.opcode = htons(1);
    memcpy(arp_frame.src_mac_addr ,src_mac , MAC_ADDR_SIZE);
    memcpy(arp_frame.src_ipv4_addr ,src_ip , IPv4_ADDR_SIZE);
    memcpy(arp_frame.dest_mac_addr ,unknown_mac , MAC_ADDR_SIZE);
    memcpy(arp_frame.dest_ipv4_addr ,ip_addr , IPv4_ADDR_SIZE);

    uint16_t type = 0x0806;
    int err;
    short correct = 0; //variable para saber si el frame recibido es el que queremos
    struct arp_frame arp_received;

    err = eth_send(iface, MAC_BCAST_ADDR, type, (unsigned char *)&arp_frame, ARP_FRAME_SIZE);
    if(err == -1){
        printf("Sending error");
        return -1;
    }

    timerms_t timer;
    long int timeout = 2000;
    timerms_reset(&timer, timeout);//iniciamos timer de 2 segundos
    long time_left = timerms_left(&timer);
    while(time_left > 1){
        correct = eth_recv(iface, mac_addr, type, (unsigned char *)&arp_received, ARP_FRAME_SIZE, 50);
        
        if(memcmp(&arp_frame.src_mac_addr, &arp_received.dest_mac_addr,     MAC_ADDR_SIZE) == 0){
            if(ntohs(arp_received.opcode) == 2){
                char req_mac_str[16];
                mac_addr_str(arp_received.src_mac_addr, req_mac_str);
                printf("Correct response received, MAC address %s\n", req_mac_str);
                memcpy(mac_addr, arp_received.src_mac_addr, MAC_ADDR_SIZE);
                return 1;
            }
        }
    
        time_left = timerms_left(&timer);        
    }
    if(correct == 0){
        printf("We haven't received the requested ARP response frame\n");
        return 0;
    }
    else if(correct == -1){
        printf("Receiveing error\n");
        return -1;
    }
    return correct;
    
}


