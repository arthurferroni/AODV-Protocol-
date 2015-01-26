#ifndef __ROUTE_HASH_TABLE_H__
#define __ROUTE_HASH_TABLE_H__
#include "defines.h"
#include "hw_addrs.h"

typedef struct route_entry {
  struct route_entry *prev;
  struct route_entry * next;
  char dest_addr[IPADDR_LEN];		    /* Dst Address                      */
  char nexthop[IF_HADDR];	            /* Next hop's HW Addr               */
  int numhops;                              /* Number of hops to destination    */
  int ifindex;                              /* Outgoing Interface Index         */
  unsigned int timestamp;                   /* Timestamp                        */
  uint8_t broadcast_id;                     /* Latest Broadcast ID from a host  */
} route_entry_t;

typedef struct route_hash_bucket {
  route_entry_t *head;
  route_entry_t *tail;
  int size;
} route_hash_bucket_t;

typedef struct route_hash_table {
  route_hash_bucket_t *buckets;
  int num_buckets;
} route_hash_table_t;


void init_route_hash_table(route_hash_table_t *hash_table, int bucket_sz);
bool add_route_entry(char* dest, char* nexthop, int numhops, 
        int ifindex, unsigned int timestamp, uint32_t broadcast_id, route_hash_table_t* hash_table);
route_entry_t* find_route_entry(char* dest, route_hash_table_t* hash_table);
void del_route_entry(char* dest, route_hash_table_t* hash_table);
void update_route_entry_timestamp(char* dest, unsigned int timestamp, route_hash_table_t* hash_table);
void print_route_entry(route_entry_t* entry);
void print_route_hash_table(route_hash_table_t* hash_table);
#endif /* __ROUTE_HASH_TABLE_H__ */
