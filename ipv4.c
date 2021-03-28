#include "ipv4.h"
#include "ipv4_route_table.h"
#include "ipv4_config.h"
#include "arp.h"
#include "eth.h"

#include <timerms.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

/* Dirección IPv4 a cero: "0.0.0.0" */
ipv4_addr_t IPv4_ZERO_ADDR = { 0, 0, 0, 0 };

//Capa IP
typedef struct ipv4_layer{
    eth_iface_t * iface; //interfaz
    ipv4_addr_t addr;    //direccion propia
    ipv4_addr_t netmask; //mascara de red
    ipv4_route_table_t * route_table;
} ipv4_layer_t;

//struct datagrama IP
typedef struct ipv4_frame{
    uint8_t version_and_len;
    uint8_t tos;               
    uint16_t total_len;
    uint16_t id;
    uint16_t flags_and_offset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    ipv4_addr_t src_ip;  //direccion de origen
    ipv4_addr_t dest_ip; //direccion de destino
    uint8_t payload[ETH_MTU - IPv4_HEADER_SIZE];//payload de eth menos la longitud de la cabecera
} ipv4_frame_t;

/* void ipv4_addr_str ( ipv4_addr_t addr, char* str );
 *
 * DESCRIPCIÓN:
 *   Esta función genera una cadena de texto que representa la dirección IPv4
 *   indicada.
 *
 * PARÁMETROS:
 *   'addr': La dirección IP que se quiere representar textualente.
 *    'str': Memoria donde se desea almacenar la cadena de texto generada.
 *           Deben reservarse al menos 'IPv4_STR_MAX_LENGTH' bytes.
 */
void ipv4_addr_str ( ipv4_addr_t addr, char* str )
{
  if (str != NULL) {
    sprintf(str, "%d.%d.%d.%d",
            addr[0], addr[1], addr[2], addr[3]);
  }
}


/* int ipv4_str_addr ( char* str, ipv4_addr_t addr );
 *
 * DESCRIPCIÓN:
 *   Esta función analiza una cadena de texto en busca de una dirección IPv4.
 *
 * PARÁMETROS:
 *    'str': La cadena de texto que se desea procesar.
 *   'addr': Memoria donde se almacena la dirección IPv4 encontrada.
 *
 * VALOR DEVUELTO:
 *   Se devuelve 0 si la cadena de texto representaba una dirección IPv4.
 *
 * ERRORES:
 *   La función devuelve -1 si la cadena de texto no representaba una
 *   dirección IPv4.
 */
int ipv4_str_addr ( char* str, ipv4_addr_t addr )
{
  int err = -1;

  if (str != NULL) {
    unsigned int addr_int[IPv4_ADDR_SIZE];
    int len = sscanf(str, "%d.%d.%d.%d", 
                     &addr_int[0], &addr_int[1], 
                     &addr_int[2], &addr_int[3]);

    if (len == IPv4_ADDR_SIZE) {
      int i;
      for (i=0; i<IPv4_ADDR_SIZE; i++) {
        addr[i] = (unsigned char) addr_int[i];
      }
      
      err = 0;
    }
  }
  
  return err;
}


/*
 * uint16_t ipv4_checksum ( unsigned char * data, int len )
 *
 * DESCRIPCIÓN:
 *   Esta función calcula el checksum IP de los datos especificados.
 *
 * PARÁMETROS:
 *   'data': Puntero a los datos sobre los que se calcula el checksum.
 *    'len': Longitud en bytes de los datos.
 *
 * VALOR DEVUELTO:
 *   El valor del checksum calculado.
 */
uint16_t ipv4_checksum ( unsigned char * data, int len )
{
  int i;
  uint16_t word16;
  unsigned int sum = 0;
    
  /* Make 16 bit words out of every two adjacent 8 bit words in the packet
   * and add them up */
  for (i=0; i<len; i=i+2) {
    word16 = ((data[i] << 8) & 0xFF00) + (data[i+1] & 0x00FF);
    sum = sum + (unsigned int) word16;	
  }

  /* Take only 16 bits out of the 32 bit sum and add up the carries */
  while (sum >> 16) {
    sum = (sum & 0xFFFF) + (sum >> 16);
  }

  /* One's complement the result */
  sum = ~sum;

  return (uint16_t) sum;
}


/*
 * ipv4_layer_t * ipv4_open(char * file_conf, char * file_conf_route)
 *
 * DESCRIPCIÓN:
 *   Esta función abre la capa IP.
 *
 * PARÁMETROS:
 *   'file_conf': Puntero a los datos sobre los que se leen los datos de la capa.
 *    'file_conf_route': Puntero a los datos sobre los que se lee la tabla de rutas.
 *
 * VALOR DEVUELTO:
 *   layer (capa inicializada)
 */

ipv4_layer_t * ipv4_open(char * file_conf, char * file_conf_route) {
    //crear layer y leer archivo de configuración
    ipv4_layer_t * layer = malloc(sizeof(ipv4_layer_t));

    char ifname[IFACE_NAME_MAX_LENGTH];        //nombre de la interfaz eth
    ipv4_addr_t addr;       //addr NUESTRA
    ipv4_addr_t netmask;    //mascara de red 
    
    //leer el fichero de configuración y sacar la interfaz, address y mascara NUESTRAS
    int err = ipv4_config_read(file_conf, ifname, addr, netmask ); 
    fflush(stdout); //porque leemos

    eth_iface_t * iface = eth_open(ifname); //abriendo interfaz
    
    if(err < 0){
        printf("Error al abrir la interfaz en la lectura del fichero de configuración");
        exit(-1);
    }
    
    //creamos y leemos la tabla de direccionamiento
    ipv4_route_table_t * route_table = ipv4_route_table_create();
    err = ipv4_route_table_read(file_conf_route, route_table);

    if(err < 0){
        printf("Error al abrir la interfaz en la lectura del fichero de la tabla de direccionamiento");
        exit(-1);
    }

    //rellenar campos de la capa en layer con los que sacamos con ipv4_config_read()
    layer->iface = iface;
    memcpy(layer->addr, addr, IPv4_ADDR_SIZE);
    memcpy(layer->netmask, netmask, IPv4_ADDR_SIZE);
    layer->route_table = route_table;

    return layer;
}



/*
 * ipv4_send (ipv4_layer_t * layer, ipv4_addr_t dst, uint8_t protocol, unsigned char * payload, int payload_len)
 *
 * DESCRIPCIÓN:
 *   Esta función manda paquetes IP.
 *
 * PARÁMETROS:
 *         'layer': Capa IP que utilizamos.
 *           'dst': direccion IP de destino a la que mandamos.
 *      'protocol': Protocolo de los paquetes que se envian.
 *       'payload': Datos que mandamos.
 *   'payload_len': Longitud de los datos que mandamos.
 *
 * VALOR DEVUELTO:
 *   err (bytes mandados)
 */

int ipv4_send (ipv4_layer_t * layer, ipv4_addr_t dst, uint8_t protocol, unsigned char * payload, int payload_len) {
    int err_value = 0;
    //buscamos la mejor ruta para llegar a nuestro objetivo
    ipv4_route_t * route = ipv4_route_table_lookup(layer->route_table, dst);
    
    //declaramos dataframe y rellenamos los campos
    ipv4_frame_t frame;
    frame.version_and_len = 0x45; //version 4 y longitud 5 (5*8 = 20) 01000101
    frame.tos = 0; //type of service
    frame.total_len = htons(payload_len + IPv4_HEADER_SIZE); 
    frame.id = htons(0);
    frame.flags_and_offset = htons(0);
    frame.ttl = 64;
    frame.protocol = protocol;
    //copiamos al frame la información de la layer y los argumentos
    memcpy(frame.src_ip, layer->addr , IPv4_ADDR_SIZE);
    memcpy(frame.dest_ip ,dst , IPv4_ADDR_SIZE);
    memcpy(frame.payload ,payload , payload_len);

    frame.checksum = 0; //no hacemos checksum
    uint16_t checksum = ipv4_checksum((unsigned char *)&frame, IPv4_HEADER_SIZE);
    frame.checksum = htons(checksum);
    //conseguimos la dirección MAC a la que mandamos llamando a arp_resolve
    mac_addr_t dst_mac; 
    int err;
    //en caso de que la dirección de destino esté en nuestra subred
    if(memcmp(IPv4_ZERO_ADDR, route->gateway_addr, IPv4_ADDR_SIZE) == 0){
        err = arp_resolve(layer->iface, layer->addr, dst, dst_mac); //buscamos la mac si está en nuestra subred
        if(err != 1){
            printf("Error al encontrar la MAC del equipo de destino al enviar el paquete IP\n");
            err_value = -1;
            return err_value;
        }
    }else{
        err = arp_resolve(layer->iface, layer->addr, route->gateway_addr, dst_mac); //buscamos la mac si está en otra subred
        if(err != 1){
            printf("Error al encontrar la MAC del siguiente salto al enviar el paquete IP\n");
            err_value = -1;
            return err_value;
        }
    }
    
    err = eth_send(layer->iface, dst_mac, 0x0800, (unsigned char *)&frame, payload_len+IPv4_HEADER_SIZE);
    if(err == -1){
        printf("Error en eth_send al enviar datagrama IP");
        err_value = -1;
        return err_value;
    }
    
    return err;
}

/*
 * ipv4_recv(ipv4_layer_t * layer, uint8_t protocol, unsigned char buffer[], ipv4_addr_t sender, int buf_len, long int timeout)
 *
 * DESCRIPCIÓN:
 *   Esta función recibe paquetes IP.
 *
 * PARÁMETROS:
 *         'layer': Capa IP que utilizamos.
 *      'protocol': Protocolo de los paquetes que se envian.
 *      'buffer[]': Donde almacenamos los datos recibidos.
 *        'sender': Datos que mandamos.
 *       'buf_len': Longitud para los datos que recibimos.
 *       'timeout': Tiempo de espera para los datos que recibimos.
 *
 * VALOR DEVUELTO:
 *   recv_bytes (bytes mandados)
 */

int ipv4_recv(ipv4_layer_t * layer, uint8_t protocol, unsigned char buffer[], ipv4_addr_t sender, int buf_len, long int timeout) {

    ipv4_frame_t frame_recv;
    mac_addr_t sender_mac;  //
    uint16_t type = 0x0800; //tipo IPv4
    int payload_len = -1;
    unsigned char eth_payload[ETH_MTU];
    int eth_payload_len = ETH_MTU;
    
    //iniciamos timer
    timerms_t timer;
    timerms_reset(&timer, timeout);
    long time_left = timerms_left(&timer);
    int recv_bytes;
    int is_my_ip;
    int is_multicast;

    //pasamos la mac del que manda a string
    mac_addr_t mac;  
    char macstr[20]; 
    eth_getaddr(layer->iface, mac);
    mac_addr_str(mac, macstr);

    do{
        // salir si se consume el timer
        time_left = timerms_left(&timer);
        // mandamos 
        recv_bytes = eth_recv(layer->iface, sender_mac, type, eth_payload, eth_payload_len, time_left);
        
        if(recv_bytes == -1){
            printf("error err_recv = -1\n");
            return -1;
        }
        else if(recv_bytes == 0)
            return 0;


        memcpy(&frame_recv, eth_payload, recv_bytes);

        /*
        char str_ip[32];
        char dest_ip_str[32];
        ipv4_addr_str(layer->addr, str_ip);
        ipv4_addr_str(frame_recv.dest_ip, dest_ip_str);

        printf("our ip: %s\nframe_recv.dest_ip: %s\n", str_ip, dest_ip_str);
        */

        is_my_ip = memcmp(frame_recv.dest_ip, layer->addr, IPv4_ADDR_SIZE) == 0;//compruebo si la dirección IP destino es la mía

        is_multicast = (frame_recv.dest_ip[0]&(0xF0)) == (0xE0);//comprobamos que los primeros 4 bits son 1110 (=E)+++++++

        if(is_my_ip){
            printf("recibimos unicast en ip\n");
        }

        if(is_multicast){
           printf("recibimos multicast en ip\n");
        }

            
        
    }while(!( (is_my_ip || is_multicast) && frame_recv.protocol == protocol));
    
    if(recv_bytes < 1){
        printf("No hemos recibido ningún paquete IP correspondiente al protocolo indicado\n");
        return 0;
    }
    // guardamos en sender la direccion IP del que manda
    memcpy(sender, frame_recv.src_ip, IPv4_ADDR_SIZE);

    int frame_len = ntohs(frame_recv.total_len);
    payload_len = frame_len - IPv4_HEADER_SIZE;
    if (buf_len > payload_len) {
        buf_len = payload_len;
    }
    // guardamos en buffer los datos recibidos
    memcpy(buffer, frame_recv.payload, buf_len);

    return recv_bytes; 
}

/*
 * ipv4_close (ipv4_layer_t * layer)
 *
 * DESCRIPCIÓN:
 *   Esta función cierra la capa IP especificada y libera la memoria
 *   de su manejador.
 *
 * PARÁMETROS:
 *   'layer': Manejador de la capa IP que se desea cerrar.
 *
 * VALOR DEVUELTO:
 *   Devuelve 0 si la capa IP se ha cerrado correctamente.
 * 
 * ERRORES:
 *   La función devuelve '-1' si se ha producido algún error. 
 */

int ipv4_close (ipv4_layer_t * layer) {
    /* 1. Liberar table de rutas (layer -> routing_table) */
    ipv4_route_table_free(layer->route_table);
    /* 2. Cerrar capa Ethernet con eth_close() */
    int err = eth_close(layer->iface);
    if(err == -1)
        printf("Error al cerrar la interfaz ethernet");

    free(layer);

    return err;
}




