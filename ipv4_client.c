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

int main ( int argc, char * argv[] ){

    char * myself = basename(argv[0]);
    if ( argc != 5 ) {
        printf("Uso: %s <ipv4><long>\n", myself);
        printf("         <ipv4>: Dirección IP de destino\n");
        printf("        <long>: Longitud de los datos enviados al servidor IP\n");
        printf("        <string>: Archivo de configuración del Cliente\n");
        printf("        <string>: Archivo de tabla de rutas del Cliente\n");
        exit(-1);
    }

    //guardar parámetros de entrada en variables
    char * ip_dest_str = argv[1];
    char * data_sent_str = argv[2];
    char * config_file = argv[3];
    char * route_file = argv[4];

    //convertir de string a ipv4
    ipv4_addr_t ip_dest;
    int err = ipv4_str_addr ( ip_dest_str, ip_dest );
    if (err < 0) {
        printf("%s: ERROR en ipv4_str_addr(\"%s\")\n", myself, ip_dest_str);
        exit(-1);
    }

    int payload_len = atoi(data_sent_str);

    ipv4_layer_t * layer = ipv4_open(config_file, route_file);
    if (layer == NULL) {
        printf("%s: ERROR en ipv4_open()\n", myself);
        exit(-1);
    }

    /* Generar payload */
    unsigned char payload[payload_len];
    int i;
    for (i=0; i<payload_len; i++) {
        payload[i] = (unsigned char) i;
    }

    ipv4_addr_t src_addr;
    unsigned char buffer[ETH_MTU - IPv4_HEADER_SIZE];

    /* Enviar trama IP al Servidor */
    printf("Enviando %d bytes al Servidor IP (%s):\n",
         payload_len, ip_dest_str);
    print_pkt(payload, payload_len, 0);
  
    err = ipv4_send(layer, ip_dest, 17, payload, payload_len);//17->UDP
    if (err == -1) {
        printf("%s: ERROR en ipv4_send()\n", myself);
        exit(-1);
    }

    int buf_len = ETH_MTU - IPv4_HEADER_SIZE;
    ipv4_addr_t sender_ip;
    long int timeout = 5000;

    /* Recibir trama IP del Servidor y procesar errores */
    while(1) {

        printf("Escuchando tramas IP\n");
    
        printf("buffer_len = %d\n", buf_len);
        printf("timeout = %ld\n", timeout);
        
        payload_len = ipv4_recv(layer, 17, buffer, sender_ip, buf_len, timeout);

        if (payload_len == -1) {
            printf("%s: ERROR en ipv4_recv()\n", myself);
            exit(-1);
        }

        if(memcmp(sender_ip, src_addr, IPv4_ADDR_SIZE) == 0){
            break;
        }
    }

    if (payload_len > 0) {
        char src_addr_str[IPv4_STR_MAX_LENGTH];
        ipv4_addr_str(src_addr, src_addr_str);    

        printf("Recibidos %d bytes del Servidor IP (%s)\n", 
               payload_len, src_addr_str);
        print_pkt(buffer, payload_len, 0);
    }

    /* Cerrar interfaz Ethernet */
    printf("Cerrando interfaz IP.\n");

    ipv4_close(layer);

    return 0;
    
}


