#ifndef __LOCAL_MSGS_H__
#define __LOCAL_MSGS_H__

#include "defines.h"

typedef struct {
	char dst_addr[IPADDR_LEN];
	uint32_t dst_port;
	uint32_t flag;
	uint32_t len;
	char data[MSG_PAYLOAD_LEN];
} send_to_odr_msg_t;

typedef struct {
	char src_addr[IPADDR_LEN];
	uint32_t src_port;
	uint32_t len;
	char data[MSG_PAYLOAD_LEN];
} recv_from_odr_msg_t;

void build_send_msg(send_to_odr_msg_t *msg, char* dst_addr, int dst_port, char* data, int flag);
void print_send_msg(send_to_odr_msg_t *msg);
void build_recv_msg(recv_from_odr_msg_t *msg, char* src_addr, int src_port, char* data);
void print_recv_msg(recv_from_odr_msg_t *msg);
void msg_send(int sock_fd, char* dst_addr, int dst_port, char* data, int flag);
void msg_recv(int sock_fd, char* msg, char* src_addr, int* src_port);
#endif /* __LOCAL_MSGS_H__ */
