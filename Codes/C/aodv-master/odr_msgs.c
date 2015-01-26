#include "odr_msgs.h"

/* Allocate a RREQ message */
rreq_msg_t * alloc_rreq_msg( uint16_t broadcast_id, char *src_addr,
                                    char *dest_addr, uint8_t hop_count, 
                                    uint8_t already_sent, uint8_t force_discover)
{
  rreq_msg_t *rreq = (rreq_msg_t *)calloc(1, sizeof(rreq_msg_t));
  
  rreq->msg_type = MSG_RREQ;
  rreq->hop_count = hop_count;
  rreq->already_sent = already_sent;
  rreq->force_discover = force_discover;
  strncpy(rreq->src_addr, src_addr, IPADDR_LEN);
  strncpy(rreq->dest_addr, dest_addr, IPADDR_LEN);
  
  return(rreq);
}

/* Free a RREQ message */
inline void free_rreq_msg(rreq_msg_t *msg)
{
  assert(msg);
  free(msg);
}

/* Allocate a rresp message */
rresp_msg_t * alloc_rresp_msg( uint16_t broadcast_id, char *src_addr,
                             char *dest_addr, uint8_t hop_count, uint8_t force_discover)
{
  rresp_msg_t *rresp = (rresp_msg_t *)calloc(1, sizeof(rresp_msg_t));
  
  rresp->msg_type = MSG_RRESP;
  rresp->hop_count = hop_count;
  rresp->force_discover = force_discover;
  strncpy(rresp->src_addr, src_addr, IPADDR_LEN);
  strncpy(rresp->dest_addr, dest_addr, IPADDR_LEN);
  
  return(rresp);
}

/* Free a rresp message */
inline void free_rresp_msg(rresp_msg_t *msg)
{
  assert(msg);
  free(msg);
}

/* Allocate a PAYLOAD message */
payload_msg_t * alloc_payload_msg(uint16_t src_port, char *src_addr,
                                         uint16_t dst_port, char *dest_addr,
                                         uint8_t hop_count, uint16_t data_len, char *data)
{
  payload_msg_t * payload =(payload_msg_t *)calloc(1, sizeof(payload_msg_t));
  payload->msg_type = MSG_PAYLOAD;
  payload->hop_count = hop_count;
  payload->src_port = src_port;
  payload->dest_port = dst_port;
  strncpy(payload->src_addr, src_addr, IPADDR_LEN);
  strncpy(payload->dest_addr, dest_addr, IPADDR_LEN);
  payload->data_len = data_len;
  payload->data = (char*)calloc(1, data_len);
  strncpy(payload->data, data, data_len);
  return payload;
}

/* Free a PAYLOAD message */
inline void free_payload_msg(payload_msg_t *msg)
{
  assert(msg);
  assert(msg->data_len);
  assert(msg->data);
  free(msg->data);
  free(msg);
}
/* Build a RREQ PDU from a rreq_msg_t message */
void build_rreq_pdu(char* pkt, rreq_msg_t *msg)
{  
  memcpy(pkt, msg, sizeof(*msg));
  ((rreq_msg_t *)pkt)->broadcast_id = htonl(msg->broadcast_id);
}

/* Parse a RREQ PDU into a rreq_msg_t message */
void parse_rreq_pdu(char *pkt , rreq_msg_t *msg)
{
  memcpy(msg, pkt, sizeof(*msg));
  msg->broadcast_id = ntohl(msg->broadcast_id);
}

/* Build a rresp PDU from a rreq_msg_t message */
void build_rresp_pdu(char* pkt, rresp_msg_t *msg)
{  
  memcpy(pkt, msg, sizeof(*msg));
  ((rresp_msg_t *)pkt)->broadcast_id = htonl(msg->broadcast_id);
}

/* Parse a rresp PDU into a rresp_msg_t message */
void parse_rresp_pdu(char *pkt , rresp_msg_t *msg)
{
  memcpy(msg, pkt, sizeof(*msg));
  msg->broadcast_id = ntohl(msg->broadcast_id);
}

/* Build a PAYLOAD PDU from a payload_msg_t message */
void build_payload_pdu(char* pkt, payload_msg_t *msg)
{  
  char *data = msg->data;
  memcpy(pkt, msg, sizeof(*msg));
  ((payload_msg_t *)pkt)->src_port  =  htons(msg->src_port);
  ((payload_msg_t *)pkt)->dest_port =  htons(msg->dest_port);
  ((payload_msg_t *)pkt)->data_len  =  htons(msg->data_len);

  printf(" msg->data_len = %d, data = %s, to = %x\n", msg->data_len, data, &(((payload_msg_t *)pkt)->data));
  strncpy((char *)(&(((payload_msg_t *)pkt)->data)), data, msg->data_len);
}

/* Parse a PAYLOAD PDU into a payload_msg_t message */
void parse_payload_pdu(char *pkt , payload_msg_t *msg)
{
  memcpy(msg, pkt, sizeof(*msg));
  msg->src_port = ntohs(msg->src_port);
  msg->dest_port = ntohs(msg->dest_port);
  msg->data_len = ntohs(msg->data_len);

  msg->data = (char*)calloc(1, msg->data_len);
  strncpy(msg->data, (char *)(&(((payload_msg_t *)pkt)->data)), msg->data_len);
}
