#include "route_hash_table.h"

int generate_key(char* s, int bucket_sz)
{
  int total = 0;
  while(*s) 
  {
    total += (int)(*s);
    s++;
  }
  return total % bucket_sz;
}

void init_route_hash_table(route_hash_table_t* hash_table, int num_buckets)
{
  assert(hash_table);
  hash_table->num_buckets = num_buckets;
  hash_table->buckets = (route_hash_bucket_t *)calloc(num_buckets, sizeof(route_hash_bucket_t));
  assert(hash_table->buckets);
}

bool add_route_entry(char* dest_addr, char* nexthop, int numhops, 
    int ifindex, unsigned int timestamp, uint32_t broadcast_id, route_hash_table_t* hash_table)
{
  int index;
  route_hash_bucket_t *hash_bucket = NULL;
  route_entry_t* entry = (route_entry_t*)calloc(1,sizeof(route_entry_t));
  if (!entry)
    return FALSE;

  /* Fill up the entry */
  strncpy(entry->dest_addr, dest_addr, IPADDR_LEN);
  memcpy(entry->nexthop, nexthop, IF_HADDR);

  entry->numhops = numhops;
  entry->ifindex = ifindex;
  entry->timestamp = timestamp;
  entry->broadcast_id = broadcast_id;

  /* Add to the Right List */
  index = generate_key(dest_addr, hash_table->num_buckets);
  hash_bucket = &(hash_table->buckets[index]);
  
  entry->prev = hash_bucket->tail;
  entry->next = NULL;
  if (hash_bucket->tail != NULL) {
    hash_bucket->tail->next = entry;
  }
  if (hash_bucket->size == 0) {
    hash_bucket->head = entry;
  }
  hash_bucket->tail = entry;
  hash_bucket->size += 1;

  return TRUE;
}

route_entry_t* find_route_entry(char* dest_addr, route_hash_table_t* hash_table)
{
  int index;
  route_hash_bucket_t *hash_bucket = NULL;
  
  /* Find the Right List */
  index = generate_key(dest_addr, hash_table->num_buckets);
  hash_bucket = &(hash_table->buckets[index]);
  
  route_entry_t* entry = hash_bucket->head;
  while (entry)
  {
    if (strcmp(entry->dest_addr, dest_addr) == 0)
      return entry;
    entry = entry->next;
  }
  return entry;
}

void del_route_entry(char* dest_addr, route_hash_table_t* hash_table) 
{
  int index;
  route_hash_bucket_t *hash_bucket = NULL;

  /* Find the Right List */
  index = generate_key(dest_addr, hash_table->num_buckets);
  hash_bucket = &(hash_table->buckets[index]);

  route_entry_t* entry = hash_bucket->head;
  while (entry)
  {
    if (strcmp(entry->dest_addr, dest_addr) == 0)
    {
      if (entry->prev != NULL) {
        entry->prev->next = entry->next;
      }
      else {
        hash_bucket->head = entry->next;
      }

      if (entry->next != NULL) {
        entry->next->prev = entry->prev;
      }
      else {
        hash_bucket->tail = entry->prev;
      }
      
      free(entry);
      hash_bucket->size -= 1;
      return;
    }
    entry = entry->next;
  }
}

inline void update_route_entry_timestamp(char* dest_addr, unsigned int timestamp, route_hash_table_t* hash_table)
{
  route_entry_t* entry = find_route_entry(dest_addr, hash_table);
  if (entry)
    entry->timestamp= timestamp;
}

void print_route_entry(route_entry_t* entry)
{
  int i = 0;
  printf("Dest:%s\t", entry->dest_addr);
  printf("Nexthop:");
  for(; i<IF_HADDR; i++) {
    printf("%.2x%s", entry->nexthop[i], (i!=IF_HADDR-1) ? ":" : "");
  }
  printf("\tIfindex:%d\tNumhops:%d\tTimestamp:%u \n", entry->ifindex, entry->numhops, entry->timestamp);
}

void print_route_hash_table(route_hash_table_t* hash_table)
{
  route_entry_t* entry = NULL;
  int i = 0;
  printf("\nRoute Hash Table:\n");
  for(;i<hash_table->num_buckets; i++)
  {
    if ((hash_table->buckets[i]).size)
    {
      printf("Bucket %d\n", i);
      entry = (hash_table->buckets[i]).head;
      while (entry) {
        print_route_entry(entry);
        entry = entry->next;
      }
      printf("\n");
    }
  }
}
