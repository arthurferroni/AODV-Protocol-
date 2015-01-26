#ifndef __SEND_QUEUE_H__
#define __SEND_QUEUE_H__
#include "defines.h"
#include "odr_msgs.h"
#include "odr.h"
#include "route_hash_table.h"


typedef struct send_node {
	struct send_node *next;
	struct send_node *prev;
        payload_msg_t *msg;
	uint32_t broadcast_id;
        uint32_t timestamp;
} send_node_t;

typedef struct send_queue {
	send_node_t* head; 
	send_node_t* tail; 
	int size;
} send_queue_t;

void init_send_queue(send_queue_t *queue);
void add_to_send_queue(send_queue_t *queue, payload_msg_t *msg, unsigned int broadcast_id);
void free_send_queue(send_queue_t *queue);
void print_send_queue(send_queue_t* queue);
void resend_from_send_queue(send_queue_t *queue, route_entry_t *route_entry, int net_sockfd);
void delete_from_send_queue(send_queue_t *queue, uint16_t port);
#endif /* __SEND_QUEUE_H__ */
