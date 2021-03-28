#include "ripv2.h"
#include "udp.h"
#include "ipv4.h"
#include "arp.h"
#include "eth.h"

#include <timerms.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>


ripv2_table_t * ripv2_table_create()
{
  ripv2_table_t * table;

  table = (ripv2_table_t *) malloc(sizeof(struct ripv2_table));
  if (table != NULL) {
    int i;

    
    for (i=0; i<RIPv2_TABLE_SIZE; i++) {
      table->entries[i] = (ripv2_entry_t *)NULL;
    }
    
  }

  return table;
}



void ripv2_route_print ( ripv2_route_t * route )
{
  if (route != NULL) {

    char ipv4_address_str[IPv4_STR_MAX_LENGTH];
    char ipv4_mask_str[IPv4_STR_MAX_LENGTH];
    char next_hop_str[IPv4_STR_MAX_LENGTH];
 
    ipv4_addr_str(route->ipv4_address, ipv4_address_str);
    ipv4_addr_str(route->ipv4_mask, ipv4_mask_str);
    ipv4_addr_str(route->next_hop, next_hop_str);


    uint32_t metric = ntohl(route->metric);
    //long int time_left = timerms_left(&route->timer);

    printf("%s/%s via %s, metric: %"PRIu32"\n", ipv4_address_str, ipv4_mask_str, next_hop_str, metric);
  }
}


void ripv2_entry_print ( ripv2_entry_t * entry )
{
  if (entry != NULL) {

    char ipv4_address_str[IPv4_STR_MAX_LENGTH];
    char ipv4_mask_str[IPv4_STR_MAX_LENGTH];
    char next_hop_str[IPv4_STR_MAX_LENGTH];
 
    ipv4_addr_str(entry->route.ipv4_address, ipv4_address_str);
    ipv4_addr_str(entry->route.ipv4_mask, ipv4_mask_str);
    ipv4_addr_str(entry->route.next_hop, next_hop_str);


    uint32_t metric = entry->route.metric;

    long int time_left = timerms_left(&entry->timer);

    printf("%s/%s via %s, metric: %"PRIu32", time left: %ld\n", ipv4_address_str, ipv4_mask_str, next_hop_str, metric, time_left);
  }
}


void ripv2_table_print(ripv2_table_t * table){

    
    for(int i = 0; i<RIPv2_TABLE_SIZE; i++){
        if(table->entries[i] != NULL)
            ripv2_entry_print(table->entries[i]);
    }

}



ripv2_entry_t* ripv2_entry_read ( char* filename, int linenum, char * line )
{
  ripv2_entry_t* entry = NULL;

  char subnet_str[256];
  char mask_str[256];
  char gw_str[256];
  uint32_t metric;
  long int time_left;

  /* Parse line: Format "<subnet> <mask> <iface> <gw>\n" */
  int params = sscanf(line, "%s %s %s %"PRIu32" %ld\n", 
	       subnet_str, mask_str, gw_str, &metric, &time_left);
  if (params != 5) {
    fprintf(stderr, "%s:%d: Invalid RIPv2 Route format: '%s' (%d params)\n",
	    filename, linenum, line, params);
    fprintf(stderr, 
	    "%s:%d: Format must be: <subnet> <mask> <gw>\n",
	    filename, linenum);
    return NULL;
  }
    
  /* Parse IPv4 route subnet address */
  ipv4_addr_t subnet;
  int err = ipv4_str_addr(subnet_str, subnet);
  if (err == -1) {
    fprintf(stderr, "%s:%d: Invalid <subnet> value: '%s'\n", 
	    filename, linenum, subnet_str);
    return NULL;
  }
  
  /* Parse IPv4 route subnet mask */
  ipv4_addr_t mask;
  err = ipv4_str_addr(mask_str, mask);
  if (err == -1) {
    fprintf(stderr, "%s:%d: Invalid <mask> value: '%s'\n",
	    filename, linenum, mask_str);
    return NULL;
  }
  
  /* Parse IPv4 route gateway */
  ipv4_addr_t gateway;
  err = ipv4_str_addr(gw_str, gateway);
  if (err == -1) {
    fprintf(stderr, "%s:%d: Invalid <gw> value: '%s'\n",
	    filename, linenum, gw_str);
    return NULL;
  }
  
  /* Create new route with parsed parameters */
  entry = ripv2_entry_create( subnet, mask, gateway , metric, time_left);
  if (entry == NULL) {
    fprintf(stderr, "%s:%d: Error creating the new route\n",
	    filename, linenum);    
  }
  
  return entry;
}




int ripv2_table_read ( char * filename, ripv2_table_t * table )
{
  int read_routes = 0;

  FILE * routes_file = fopen(filename, "r");
  if (routes_file == NULL) {
    printf("Error opening input RIPv2 Routes file.\n");
    return -1;
  }

  int linenum = 0;
  char line_buf[1024];
  int err = 0;

  while ((! feof(routes_file)) && (err==0)) {

    linenum++;

    /* Read next line of file */
    char* line = fgets(line_buf, 1024, routes_file);
    if (line == NULL) {
      break;
    }

    /* If this line is empty or a comment, just ignore it */
    if ((line_buf[0] == '\n') || (line_buf[0] == '#')) {
      err = 0;
      continue;
    }

    /* Parse route from line */
    ripv2_entry_t* new_entry = ripv2_entry_read(filename, linenum, line);
    if (new_entry == NULL) {
      err = -1;
      break;
    }
      
    /* Add new route to Route Table */
    if (table != NULL) {
      err = ripv2_table_add(table, new_entry);
      if (err >= 0) {
	err = 0;
	read_routes++;
      }
    }
  } /* while() */

  if (err == -1) {
    read_routes = -1;
  }

  /* Close IP Route Table file */
  fclose(routes_file);

  return read_routes;
}



ripv2_entry_t * ripv2_entry_create
( ipv4_addr_t subnet, ipv4_addr_t mask, ipv4_addr_t gw , uint32_t metric, long int time_left)
{
  ripv2_entry_t * entry = (ripv2_entry_t *) malloc(sizeof(struct ripv2_entry));

  if ((entry != NULL) && 
      (subnet != NULL) && (mask != NULL) && (gw != NULL) && (time_left != 0)) {
    memcpy(entry->route.ipv4_address, subnet, IPv4_ADDR_SIZE);
    memcpy(entry->route.ipv4_mask, mask, IPv4_ADDR_SIZE);
    //strncpy(route->iface, iface, IFACE_NAME_MAX_LENGTH);
    memcpy(entry->route.next_hop, gw, IPv4_ADDR_SIZE);
    entry->route.metric = metric;
    timerms_t timer;
    timerms_reset(&timer, time_left);
    entry->timer = timer;
    entry->route.family = 0;
    entry->route.tag = 0;
  }
  
  return entry;
}


int ripv2_table_add ( ripv2_table_t * table, ripv2_entry_t * entry )
{
  int entry_index = -1;

  if (table != NULL) {
    /* Find an empty place in the route table */
    int i;
    long int time_left;
    timerms_t timer;

    for (i=0; i<RIPv2_TABLE_SIZE; i++) {
      timer = entry->timer;
      time_left = timerms_left(&timer);
      if (table->entries[i] == NULL || time_left <= 0 ) {
        table->entries[i] = entry;
        entry_index = i;
        break;
      }
    }
  }

  return entry_index;
}


void ripv2_table_free ( ripv2_table_t * table )
{
  if (table != NULL) {
    int i;
    for (i=0; i<RIPv2_TABLE_SIZE; i++) {
      ripv2_entry_t * entry_i = table->entries[i];
      if (entry_i != NULL) {
        table->entries[i] = NULL;
        ripv2_entry_free(entry_i);
      }
    }
    free(table);
  }
}



void ripv2_entry_free ( ripv2_entry_t * entry )
{
  if (entry != NULL) {
    free(entry);
  }
}









