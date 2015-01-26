#ifndef __ODR_MSGS_H__
#define __ODR_MSGS_H__

#include "defines.h"

#define MSG_RREQ        0x00
#define MSG_RRESP       0x01
#define MSG_PAYLOAD     0x02

typedef struct rreq_msg {
	uint8_t msg_type;
	uint8_t force_discover;
	uint8_t hop_count;
	uint8_t already_sent;
        uint32_t broadcast_id;
	char src_addr[IPADDR_LEN];
	char dest_addr[IPADDR_LEN];
} rreq_msg_t;

typedef struct rresp_msg {
	uint8_t msg_type;
	uint8_t force_discover;
	uint8_t hop_count;
	uint8_t pad[1];
        uint32_t broadcast_id;
	char src_addr[IPADDR_LEN];
	char dest_addr[IPADDR_LEN];
} rresp_msg_t;

typedef struct payload_msg {
	uint8_t     msg_type;
        uint8_t     hop_count;
        uint8_t     pad[2];
	uint16_t    src_port;
	uint16_t    dest_port;
	char src_addr[IPADDR_LEN];
	char dest_addr[IPADDR_LEN];
	uint16_t data_len;
	char* data;
} payload_msg_t;

rreq_msg_t * alloc_rreq_msg( uint16_t broadcast_id, char *src_addr,
                                    char *dst_addr, uint8_t hop_count, 
                                    uint8_t already_sent, uint8_t force_discover);
inline void free_rreq_msg(rreq_msg_t *msg);

rresp_msg_t * alloc_rresp_msg( uint16_t broadcast_id, char *src_addr,
                             char *dst_addr, uint8_t hop_count, uint8_t force_discover);
inline void free_rresp_msg(rresp_msg_t *msg);

payload_msg_t * alloc_payload_msg(uint16_t src_port, char *src_addr,
                                         uint16_t dst_port, char *dst_addr,
                                         uint8_t hop_count, uint16_t data_len, char *data);
inline void free_payload_msg(payload_msg_t *msg);

void build_rreq_pdu(char* pkt, rreq_msg_t *msg);
void parse_rreq_pdu(char *pkt , rreq_msg_t *msg);
void build_rresp_pdu(char* pkt, rresp_msg_t *msg);
void parse_rresp_pdu(char *pkt , rresp_msg_t *msg);
void build_payload_pdu(char* pkt, payload_msg_t *msg);
void parse_payload_pdu(char *pkt , payload_msg_t *msg);
#endif /* __ODR_MSGS_H_ */
