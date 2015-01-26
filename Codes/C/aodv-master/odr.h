#ifndef __ODR_H__
#define __ODR_H__

#include "hw_addrs.h"
#include "odr_msgs.h"

extern hwa_info_t *hwa_head;

void send_payload(int net_sockfd, int egress_ifindex, char* src_mac, char* nexthop_mac, payload_msg_t *payload);
char *  get_mac_from_ifindex(hwa_info_t *head, int if_index);
#endif /* __ODR_MSGS_H_ */
