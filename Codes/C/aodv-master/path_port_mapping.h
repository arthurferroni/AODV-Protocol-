#ifndef __PATH_PORT_MAPPING_H__
#define __PATH_PORT_MAPPING_H___

#include "defines.h"

#define PURGE_THRESHOLD 50
#define TTL_INFINITE    0

typedef struct path_port_node {
	struct path_port_node *prev;
	struct path_port_node *next;
	int port;
	unsigned int ttl;
	char path[PATH_LEN];
} path_port_node_t;

typedef struct _path_port_list {
	path_port_node_t *head;
	path_port_node_t *tail;
	int size;
} path_port_list_t;

/* Externs */
void init_path_port_list(path_port_list_t* list);
int  add_to_path_port_list(path_port_list_t* list, char* path, int port, unsigned int ttl);

path_port_node_t* find_path_port_from_port(path_port_list_t* list, int port);
path_port_node_t* find_path_port_from_path(path_port_list_t* list, char *path);

void free_path_port_list(path_port_list_t* list);
void print_path_port_list(path_port_list_t* list);

#endif /* __PATH_PORT_MAPPING_H___ */
