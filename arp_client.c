#include "eth.h"
#include "ipv4.h"
#include "arp.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libgen.h>

int main ( int argc, char * argv[] ){

    char * myself = basename(argv[0]);
    if ( argc != 3 ) {
        printf("Uso: %s <iface> <ipv4>\n", myself);
        printf("       <iface>: Nombre de la interfaz Ethernet\n");
        printf("         <ipv4>: Dirección IP cuya MAC queremos obtener\n");
        exit(-1);
    }
    
    //guardar parámetros de entrada en variables
    char * iface_name = argv[1];
    char * ip_req_str = argv[2];

    //convertir de string a ipv4
    ipv4_addr_t ip_req;
    int err = ipv4_str_addr ( ip_req_str, ip_req );
    if (err < 0) {
        fprintf(stderr, "%s: ERROR en ipv4_str_addr(\"%s\")\n", myself, ip_req_str);
        exit(-1);
    }
    
    //abrir ethernet interface y usar arp resolve
    eth_iface_t * eth_iface = eth_open(iface_name);
    if (eth_iface == NULL) {
        fprintf(stderr, "%s: ERROR en eth_open(\"%s\")\n", myself, iface_name);
        exit(-1);
    }
    mac_addr_t mac_addr;
    
    err = arp_resolve( eth_iface, ip_req, mac_addr);
    if(err < 1){
        printf(stderr, "%s: ERROR en arp_resolve\n", myself);
        exit(-1);
    }

    char mac_str[16];
    mac_addr_str(mac_addr, mac_str);
    printf("%s -> %s\n", ip_req_str, mac_str);
    
    eth_close(eth_iface);

}
