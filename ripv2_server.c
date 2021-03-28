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
        printf("        <string>: Archivo de configuración del Servidor\n");
        printf("        <string>: Archivo de tabla de rutas IP del Servidor\n");
        printf("        <string>: Archivo de tabla de rutas RIP del Servidor\n");
        exit(-1);
    }

    char * config_file = argv[1];
    char * route_file_ip = argv[2];
    char * route_file_rip = argv[3];

    udp_layer_t * layer = udp_open(config_file, route_file_ip, 0);
    if (layer == NULL) {
        printf("ERROR en udp_open()\n");
        exit(-1);
    }

    ripv2_table_t * table = ripv2_table_create();

    int read_entries = ripv2_table_read(route_file_rip, table);
    if(read_entries < 0){
        printf("Error al leer la tabla rip");
        exit(-1);
    }
    
    //ripv2_table_print(table);

    int i, recv_bytes, entries_recv;
    long int timeout, time_left;
    uint16_t ripv2_port = 520;

    while(1){
        timeout = 180000;

        for(i = 0; i<RIPv2_TABLE_SIZE; i++){
            if(table->entries[i] != NULL){

                time_left = timerms_left(&table->entries[i]->timer);

                    if(time_left <= 0 && table->entries[i]->route.metric < 16){ //garbage collection timer
                        ripv2_entry_free(table->entries[i]);
                        table->entries[i] = NULL;
                    }

                    else if(time_left<timeout)
                        timeout = time_left;
            }
        }
        printf("timeout: %ld\n", timeout);

        unsigned char buffer[ETH_MTU - IPv4_HEADER_SIZE - UDP_HEADER_SIZE];
        ripv2_frame_t frame_recv;

        recv_bytes = udp_recv(layer, IPv4_ZERO_ADDR, &ripv2_port, buffer, sizeof(ripv2_frame_t), timeout);
        if (recv_bytes < 1) {
            printf("ERROR en udp_recv()\n");
            exit(-1);
        }
        printf("recv bytes: %d\n", recv_bytes);


        memcpy(&frame_recv, &buffer, recv_bytes - IPv4_HEADER_SIZE - UDP_HEADER_SIZE);

        entries_recv = (recv_bytes - RIPv2_HEADER_SIZE - IPv4_HEADER_SIZE - UDP_HEADER_SIZE)/sizeof(ripv2_route_t);

        printf("entries_recv: %d\n", entries_recv);
        for(int i = 0; i<entries_recv; i++){

            ripv2_route_print(&frame_recv.routes[i]);

        }

    }

    ripv2_table_free(table);

}
