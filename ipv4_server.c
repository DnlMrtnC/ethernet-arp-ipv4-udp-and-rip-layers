#include "ipv4.h"
#include "ipv4_route_table.h"
#include "ipv4_config.h"
#include "arp.h"
#include "eth.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

int main ( int argc, char * argv[] ){
    //fflush(stdout);

    char * myself = basename(argv[0]);
    if ( argc != 4 ) {
        printf("Uso: %s <ipv4><long>\n", myself);
        printf("         <ipv4>: Dirección IP de destino\n");
        printf("        <string>: Archivo de configuración del Servidor\n");
        printf("        <string>: Archivo de tabla de rutas del Servidor\n");
        exit(-1);
    }


    //guardar parámetros de entrada en variables
    char * ip_dest_str = argv[1];
    char * config_file = argv[2];
    char * route_file = argv[3];

    //convertir de string a ipv4
    ipv4_addr_t ip_dest;
    int err = ipv4_str_addr(ip_dest_str, ip_dest);
    if (err < 0) {
        printf("%s: ERROR en ipv4_str_addr(\"%s\")\n", myself, ip_dest_str);
        exit(-1);
    }

    ipv4_layer_t * layer = ipv4_open(config_file, route_file);
    if (layer == NULL) {
        printf("%s: ERROR en ipv4_open()\n", myself);
        exit(-1);
    }

    unsigned char buffer[ETH_MTU - IPv4_HEADER_SIZE];
    int buf_len = ETH_MTU - IPv4_HEADER_SIZE;
    ipv4_addr_t sender_ip;
    int payload_len;
    char src_addr_str[IPv4_STR_MAX_LENGTH];
    int len;
/*
    timerms_t timer;
    long int timeout = 5000;
    long time_left;
    timerms_reset(&timer, timeout);//iniciamos timer de 2 segundos
    time_left = timerms_left(&timer);
*/

    while(1) {

        /* Recibir trama IP del Cliente */
        
        
        printf("Escuchando tramas IP\n");

        while(1){//bucle para recibir
            payload_len = ipv4_recv(layer, 17, buffer, sender_ip, buf_len, 2000);
            if(payload_len > 1)
                break;
        }

        if (payload_len == -1) {
            printf("%s: ERROR en ipv4_recv()\n", myself);
        }

        ipv4_addr_str(sender_ip, src_addr_str);
    
        printf("Recibidos %d bytes del Cliente IP (%s):\n", 
               payload_len, src_addr_str);
        //print_pkt(buffer, payload_len, 0);

        /* Enviar la misma trama Ethernet de vuelta al IP */
        printf("Enviando %d bytes al Cliente IP (%s):\n",
               payload_len, src_addr_str);
        //print_pkt(buffer, payload_len, 0);

        len = ipv4_send(layer, sender_ip, 17, buffer, buf_len);//deberia ser payload_len
        if (len == -1) {
            printf("%s: ERROR en ipv4_send()\n", myself);
        }

    }

    /* Cerrar interfaz IP */
    printf("Cerrando interfaz IP.\n");

    ipv4_close(layer);

    return 0;

}


