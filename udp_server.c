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

int main ( int argc, char * argv[] ){
    //fflush(stdout);

    char * myself = basename(argv[0]);
    if ( argc != 4 ) {
        printf("Uso: %s <string> <string> <int>\n", myself);
        printf("        <string>: Archivo de configuración del Servidor\n");
        printf("        <string>: Archivo de tabla de rutas del Servidor\n");
        printf("         <int>: puerto del servidor\n");
        exit(-1);
    }
    
    //guardar parámetros de entrada en variables
    char * config_file = argv[1];
    char * route_file = argv[2];
    int server_port = atoi(argv[3]);

    //convertir de string a ipv4
    udp_layer_t * layer = udp_open(config_file, route_file, server_port);
    if (layer == NULL) {
        printf("%s: ERROR en udp_open()\n", myself);
        exit(-1);
    }

    int payload_len = 0;
    unsigned char buffer[ETH_MTU - IPv4_HEADER_SIZE - UDP_HEADER_SIZE];
    int buf_len = sizeof(buffer);
    ipv4_addr_t src_addr;
    uint16_t src_port;
    char src_addr_str[IPv4_STR_MAX_LENGTH] = "0.0.0.0";
    int sent_bytes = 0;
    
    printf("************Servidor udp*************** \n");
    while(1){
    
        //duda de src-port pointer o &
        payload_len = udp_recv(layer,src_addr,&src_port, buffer, buf_len, -1);
        if (payload_len < 1){
            printf("ERROR en udp_recv()\n");
            break;
        }
        ipv4_addr_str(src_addr, src_addr_str);
    
        printf("Recibidos %d bytes del Cliente UDP (%s):\n", 
               payload_len, src_addr_str);
        //print_pkt(buffer, payload_len, 0);

        /* Enviar la misma trama Ethernet de vuelta al IP */
        printf("Enviando %d bytes al Cliente UDP (%s):\n", payload_len, src_addr_str);
        //print_pkt(buffer, payload_len, 0);

        //src_port = ntohs(src_port);
        printf("source port enviado en server %d\n", src_port);
    
        sent_bytes = udp_send(layer, src_addr, src_port, buffer, payload_len);
        if (sent_bytes == -1) {
            printf("%s: ERROR en udp_send()\n", myself);
        }

    }
    
    /* Cerrar interfaz IP */
    printf("Cerrando interfaz UDP.\n");

    udp_close(layer);

    return 0;    

}













