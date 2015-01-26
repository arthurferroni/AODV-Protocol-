#include "path_port_mapping.h"
#include "defines.h"

void init_path_port_list(path_port_list_t* list)
{
  list->head = NULL;
  list->tail = NULL;
  list->size = 0;
}

static inline int is_purge_needed(path_port_list_t* list)
{
  return (list->size > PURGE_THRESHOLD);
}

static inline unsigned int get_current_seconds()
{
  struct timeval tv; 
  Gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

int purge_list(path_port_list_t* list)
{
  path_port_node_t* node = list->head, *tmp_node = NULL;
  unsigned int curr_time = get_current_seconds();
  int purge_count = 0;

  while (node)
  {
    if ((node->ttl == TTL_INFINITE) || ((curr_time - node->ttl) < MAX_TTL))
    {
      node = node->next;
      continue;
    }
    else
    {
        printf("Purging Port:%d Path:%s TTL:%u\n", node->port, node->path, node->ttl);
        tmp_node = node;
        if (node->next)
        {
          node->next->prev = node->prev;
        }
        else
          list->tail = node->prev;

        if (node->prev)
          node->prev->next = node->next;
        else
          list->head = node->next;
        
        node = node->next;
        free(tmp_node);
        list->size -= 1;
        purge_count+=1;
    }
  }
  return (purge_count);
}

path_port_node_t* find_path_port_from_port(path_port_list_t* list, int port)
{
  path_port_node_t* node;
  
  if (is_purge_needed(list))
    (void)purge_list(list);

  node = list->head;
  
  while (node)
  {
    if (node->port == port)
    {
      return node;
    }
    node = node->next;
  }
  return node;
}

path_port_node_t* find_path_port_from_path(path_port_list_t* list, char *path)
{
  path_port_node_t* node;

  if (is_purge_needed(list))
    (void)purge_list(list);
  
  node = list->head;
  
  while (node)
  {
    if (!strcmp(node->path, path))
    {
      return node;
    }
    node = node->next;
  }
  return node;
}

bool add_to_path_port_list(path_port_list_t* list, char* path, int port, unsigned int ttl)
{
  path_port_node_t* node = (path_port_node_t *)calloc(1, sizeof(path_port_node_t));
  
  assert(node);

  node->port = port;
  strncpy(node->path, path, sizeof(node->path));
  node->ttl = ttl;

  /* Add to tail */
  node->prev = list->tail;
  node->next = NULL;
  if (list->tail != NULL)
  {
    list->tail->next = node;
  }

  if (list->size == 0)
  {
    list->head = node;
  }

  list->tail = node;
  list->size += 1;

  if (is_purge_needed(list))
    (void)purge_list(list);
    
  return TRUE;
}

void free_path_port_list(path_port_list_t* list)
{
  path_port_node_t* node = list->head;
  path_port_node_t* tmp_node;
  while (node)
  {
    tmp_node = node;
    node = node->next;
    free(tmp_node);
  }
}

bool inline is_empty_path_port_list(path_port_list_t* list)
{
  return (list->size == 0);
}

void print_path_port_list(path_port_list_t* list)
{
  path_port_node_t* node = list->head;
  printf("Port Path Table:\n");
  while (node != NULL)
  {
    printf("Port:%d Path:%s TTL:%u\n", node->port, node->path, node->ttl);
    node = node->next;
  }
  printf("\n");
}
