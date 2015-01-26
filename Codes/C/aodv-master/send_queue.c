#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "send_queue.h"

inline void init_send_queue(send_queue_t *queue)
{
  memset(queue, 0, sizeof(send_queue_t));
}

static inline unsigned int get_current_seconds()
{
  struct timeval tv; 
  Gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

void add_to_send_queue(send_queue_t *queue, payload_msg_t *msg, unsigned int broadcast_id)
{
  printf("add_to_send_queue: Add payload destined to %s\n into Sent Queue\n", msg->dest_addr); 
  send_node_t* entry = (send_node_t *)calloc(1, sizeof(send_node_t));
  /* Add the entry */
  entry->msg = msg;
  entry->broadcast_id = broadcast_id;
  entry->timestamp = get_current_seconds();

  /* Adjust the queue */
  entry->prev = queue->tail;

  if (queue->tail) {
    queue->tail->next = entry;
  }
  if (0 == queue->size) {
    queue->head = entry;
  }

  queue->tail = entry;
  queue->size += 1;
}

void resend_from_send_queue(send_queue_t *queue, route_entry_t *route_entry, int net_sockfd)
{
  send_node_t *node = queue->head, *tmp_node = NULL;
  unsigned int curr_time = get_current_seconds();

  while (node)
  {
    if ((curr_time - node->timestamp) < RRESP_TIMEOUT)
    {
      /* If not related to the discovered route, continue */
      if (strcmp(route_entry->dest_addr, node->msg->dest_addr))
      {
        node = node->next;
        continue;
      }
      else
      {
        /* Send the Payload */
        char *local_mac = get_mac_from_ifindex(hwa_head, route_entry->ifindex);
        send_payload(net_sockfd, route_entry->ifindex, local_mac, route_entry->nexthop, node->msg);
      }
    }
    else
    {
        printf("Timed out waiting for RRESP, purging entry: [dest: %s id %d]\n", 
                            node->msg->dest_addr, node->broadcast_id);
    }

    /* Remove the entry from the queue */
    tmp_node = node;
    if (node->next)
    {
      node->next->prev = node->prev;
    }
    else
      queue->tail = node->prev;

    if (node->prev)
      node->prev->next = node->next;
    else
      queue->head = node->next;

    node = node->next;
    free_payload_msg(tmp_node->msg);
    free(tmp_node);
    queue->size -= 1;
  }
}

void delete_from_send_queue(send_queue_t *queue, uint16_t port)
{
  send_node_t* entry = queue->head;

  if (!entry)
    return;

  /* Special Case where the entry is the first in the queue */
  if(entry->msg->src_port == port) 
  {
    queue->head = entry->next;

    if (queue->head)
      queue->head->prev = NULL;

    queue->size -= 1;

    if (0 == queue->size)
      queue->tail = NULL;

    free_payload_msg(entry->msg);
    free(entry);
    return;
  }

  entry = entry->next;
  while (entry) {
    if(entry->msg->src_port == port) 
    {
      entry->prev->next = entry->next;
      if (entry->next) {
        entry->next->prev = entry->prev;
      }
      else {
        queue->tail = entry->prev;
      }
      queue->size -= 1;
        
      free_payload_msg(entry->msg);
      free(entry);
      break;
    }
    entry = entry->next;
  }
}

void print_send_queue(send_queue_t* queue)
{
  send_node_t* entry = queue->head;
  printf("Send Queue Size: %d\n", queue->size);
  printf("Entries:\n");
  while(entry)
  {
    printf("[%s , %d]\n", entry->msg->dest_addr, entry->broadcast_id);
    entry = entry->next;
  }
}

void free_send_queue_t(send_queue_t* queue)
{
  send_node_t* entry = queue->head;
  send_node_t* curr;

  while (entry)
  {
    curr = entry;
    entry = entry->next;
    free_payload_msg(entry->msg);
    free(curr);
  }
}
