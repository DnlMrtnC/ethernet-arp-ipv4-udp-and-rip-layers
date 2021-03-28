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

    char * myself = basename(argv[0]);
    if ( argc != 6 ) {
        printf("Uso: %s <ipv4><long>\n", myself);
        printf("         <ipv4>: Dirección IP de destino\n");
        printf("        <long>: Longitud de los datos enviados al servidor IP\n");
        printf("        <string>: Archivo de configuración del Cliente\n");
        printf("        <string>: Archivo de tabla de rutas del Cliente\n");
        printf("        <int>: Archivo de tabla de rutas del Cliente\n");
        exit(-1);
    }

    
    //guardar parámetros de entrada en variables
    char * ip_dest_str = argv[1];
    char * data_sent_str = argv[2];
    char * config_file = argv[3];
    char * route_file = argv[4];
    uint16_t dst_port = atoi(argv[5]);

    //convertir de string a ipv4
    ipv4_addr_t ip_dest;
    int err = ipv4_str_addr ( ip_dest_str, ip_dest );
    if (err < 0) {
        printf("%s: ERROR en ipv4_str_addr(\"%s\")\n", myself, ip_dest_str);
        exit(-1);
    }

    int payload_len = atoi(data_sent_str);

    //Abrimos layer UDP
    udp_layer_t * layer = udp_open(config_file, route_file, 0);
    if (layer == NULL) {
        printf("%s: ERROR en udp_open()\n", myself);
        exit(-1);
    }

    /* Generar payload */
    unsigned char payload[payload_len];
    int i;
    for (i=0; i<payload_len; i++) {
        payload[i] = (unsigned char) i;
    }

    ipv4_addr_t src_addr;
    unsigned char buffer[ETH_MTU - IPv4_HEADER_SIZE - UDP_HEADER_SIZE];

    int bytes_sent = udp_send(layer, ip_dest, dst_port, payload, payload_len);
    if (bytes_sent == -1) {
        printf("%s: ERROR en udp_send()\n", myself);
        exit(-1);
    }
    
    /* Recibir trama UDP del Servidor y procesar errores */

    printf("Recibiendo tramas UDP\n");
    int recv_bytes = udp_recv(layer, src_addr, &dst_port, buffer, payload_len, 2000);
        

    if (recv_bytes < 1) {
        printf("%s: ERROR en udp_recv()\n", myself);
        exit(-1);
    }
        
    printf("Trama UDP recibida\n");
    print_pkt(buffer, recv_bytes, 0);
    
    /* Cerrar interfaz Ethernet */
    printf("Cerrando interfaz UDP.\n");

    udp_close(layer);

    return 0;










}
