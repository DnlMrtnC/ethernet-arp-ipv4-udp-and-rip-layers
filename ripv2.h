#ifndef _RIPV2_H
#define _RIPV2_H

#include <stdint.h>
#include <timerms.h>
#include "ipv4.h"

#define RIPv2_TABLE_SIZE 25
#define RIPv2_HEADER_SIZE 4

typedef struct ripv2_route{

    uint16_t family;
    uint16_t tag;
    ipv4_addr_t ipv4_address;
    ipv4_addr_t ipv4_mask;
    ipv4_addr_t next_hop;
    uint32_t metric;
    // timer - solo en las entradas de la tabla, no en las entradas de mensajes
}ripv2_route_t;

typedef struct ripv2_frame{

    uint8_t type;
    uint8_t version;
    uint16_t routing_domain;
    ripv2_route_t routes[25];

}ripv2_frame_t;

typedef struct ripv2_entry{

    ripv2_route_t route;
    timerms_t timer;

}ripv2_entry_t;

typedef struct ripv2_table{

    ripv2_entry_t * entries[RIPv2_TABLE_SIZE];

}ripv2_table_t;

ripv2_table_t * ripv2_table_create();

void ripv2_route_print ( ripv2_route_t * route );

void ripv2_entry_print ( ripv2_entry_t * entry );

void ripv2_table_print(ripv2_table_t * table);

ripv2_entry_t* ripv2_entry_read ( char* filename, int linenum, char * line );

int ripv2_table_read ( char * filename, ripv2_table_t * table );

ripv2_entry_t * ripv2_entry_create
( ipv4_addr_t subnet, ipv4_addr_t mask, ipv4_addr_t gw , uint32_t metric, long int time_left);

int ripv2_table_add ( ripv2_table_t * table, ripv2_entry_t * entry );

void ripv2_table_free ( ripv2_table_t * table );

void ripv2_entry_free ( ripv2_entry_t * entry );


#endif /* _RIPV2_H */
