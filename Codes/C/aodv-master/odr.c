#include "path_port_mapping.h"
#include "defines.h"
#include "unp.h"
#include "hw_addrs.h"
#include "local_msgs.h"
#include "route_hash_table.h"
#include "odr_msgs.h"
#include "send_queue.h"
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

static char host_name[HOSTNAME_LEN];
hwa_info_t *hwa_head =  NULL;
route_hash_table_t route_hash_table;
send_queue_t send_queue;
path_port_list_t path_port_list;
int stale_timeout = DFLT_STALENESS;

static inline unsigned int get_current_seconds()
{
  struct timeval tv; 
  Gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

int generate_ephemeral_port()
{
	static int port = 49777;
	return port++;
}

unsigned int generate_broadcast_id()
{
	static unsigned int id = 1;
	return id++;
}

/* Get the MAC address given an IfIndex */
char *  get_mac_from_ifindex(hwa_info_t *head, int if_index)
{
  hwa_info_t *iter = NULL;

  for(iter = head; iter; iter = iter->hwa_next)
  {
    if(iter->ignore)
      continue;

    if (iter->if_index == if_index)
    {
      return (iter->if_haddr);
    }
  }
  return "";
}

/* Print Pretty */
void print_odr_send_details(uint8_t msg_type, char* nexthop_mac, char *src_addr, char* dst_addr) 
{
  int i = 0;
  char src_node[HOSTNAME_LEN], dest_node[HOSTNAME_LEN], *type = "Unknown";
  unsigned char *mac = (unsigned char *)nexthop_mac;
  strncpy(src_node, get_host_name(src_addr), sizeof(src_node));
  strncpy(dest_node, get_host_name(dst_addr), sizeof(dest_node));

  if (msg_type == MSG_RREQ)
  {
    type = "Request";
  }
  else if (msg_type == MSG_RRESP)
  {
    type = "Response";
  }
  else if (msg_type == MSG_PAYLOAD)
  {
    type = "Payload";
  }

  printf("ODR at node %s : sending frame hdr  src: %s, dest: ", host_name, host_name);
  for (i = 0; i < ETH_ALEN; i++) {
    printf("%02x%s", *(mac+i), (i == ETH_ALEN-1) ? "" : ":");
  }
  printf("\n");
  printf("\t\t  ODR msg type: %s, src node: %s, dest node: %s\n\n", type,
      src_node, dest_node);
}

/* Send RREQ packets out all interfaces save exclude_ifIndex */
void send_rreq(int net_sockfd, int exclude_ifIndex, rreq_msg_t *rreq)
{
  struct sockaddr_ll sock_addr;
  static char broadcast_mac[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff}; 
  static char packet[ETH_FRAME_LEN];
  hwa_info_t *iter = NULL;

  sock_addr.sll_family = PF_PACKET;
  sock_addr.sll_protocol = htons(UNIQ_PROTO);

  sock_addr.sll_hatype = ARPHRD_ETHER;
  sock_addr.sll_pkttype = PACKET_BROADCAST;
  sock_addr.sll_halen = ETH_ALEN;
  memcpy(sock_addr.sll_addr, broadcast_mac, 6);

  memcpy(packet, broadcast_mac, 6);
  *(uint16_t*)(packet+ 2*ETH_ALEN) = htons(UNIQ_PROTO);

  build_rreq_pdu(packet + ETH_HLEN, rreq);

  for(iter = hwa_head; iter; iter = iter->hwa_next)
  {
    if(iter->ignore)
      continue;
    
    if (iter->if_index == exclude_ifIndex)
      continue;

    /* Update the Egress Interface */
    sock_addr.sll_ifindex = iter->if_index;

    /* Update the Source MAC */
    memcpy(packet+ETH_ALEN, iter->if_haddr, ETH_ALEN);

    print_odr_send_details(MSG_RREQ, broadcast_mac, rreq->src_addr, rreq->dest_addr);
    Sendto(net_sockfd, packet, sizeof(packet), 0, (SA*)&sock_addr, sizeof(sock_addr));
  }
}

/* Send RRESP packet out egress_ifindex */
void send_rresp(int net_sockfd, int egress_ifindex, char* src_mac, char* nexthop_mac, rresp_msg_t *rresp)
{
  struct sockaddr_ll sock_addr;
  static char packet[ETH_FRAME_LEN];

  memset(packet, 0, sizeof(packet)); 

  sock_addr.sll_family = PF_PACKET;
  sock_addr.sll_protocol = htons(UNIQ_PROTO);
  sock_addr.sll_hatype = ARPHRD_ETHER;
  sock_addr.sll_ifindex = egress_ifindex;
  sock_addr.sll_pkttype = PACKET_OTHERHOST;
  sock_addr.sll_halen = ETH_ALEN;

  memcpy(sock_addr.sll_addr, nexthop_mac, ETH_ALEN);

  /* Fill the Ethernet Header */
  memcpy(packet, nexthop_mac, ETH_ALEN);
  memcpy(packet+ETH_ALEN, src_mac, ETH_ALEN);
  *(uint16_t*)(packet + 2*ETH_ALEN) = htons(UNIQ_PROTO);

  /* Build PDU */
  build_rresp_pdu(packet+ETH_HLEN, rresp);

  print_odr_send_details(MSG_RRESP, nexthop_mac, rresp->src_addr, rresp->dest_addr);
  Sendto(net_sockfd, packet, ETH_FRAME_LEN, 0, (SA*)&sock_addr, sizeof(sock_addr));
}

/* Send MSG_PAYLOAD packet out egress_ifindex */
void send_payload(int net_sockfd, int egress_ifindex, char* src_mac, char* nexthop_mac, payload_msg_t *payload)
{	
  struct sockaddr_ll sock_addr;
  static char packet[ETH_FRAME_LEN];

  memset(packet, 0, sizeof(packet)); 

  sock_addr.sll_family = PF_PACKET;
  sock_addr.sll_protocol = htons(UNIQ_PROTO);
  sock_addr.sll_hatype = ARPHRD_ETHER;
  sock_addr.sll_ifindex = egress_ifindex;
  sock_addr.sll_pkttype = PACKET_OTHERHOST;
  sock_addr.sll_halen = ETH_ALEN;

  memcpy(sock_addr.sll_addr, nexthop_mac, ETH_ALEN);

  /* Fill the Ethernet Header */
  memcpy(packet, nexthop_mac, ETH_ALEN);
  memcpy(packet+ETH_ALEN, src_mac, ETH_ALEN);
  *(uint16_t*)(packet + 2*ETH_ALEN) = htons(UNIQ_PROTO);

  /* Build PDU */
  build_payload_pdu(packet+ETH_HLEN, payload);

  print_odr_send_details(MSG_PAYLOAD, nexthop_mac, payload->src_addr, payload->dest_addr);
  Sendto(net_sockfd, packet, ETH_FRAME_LEN, 0, (SA*)&sock_addr, sizeof(sock_addr));
}

void handle_rreq(char* pkt, int local_ifindex, char* canonical_addr, int net_sockfd)
{
  rreq_msg_t rreq_msg;
  route_entry_t* route_entry = NULL;
  uint32_t curr_time = 0;
  bool need_forward = TRUE;
  char neighbor_mac[ETH_ALEN];
  char *local_mac = get_mac_from_ifindex(hwa_head, local_ifindex);

  memcpy(neighbor_mac, pkt + ETH_ALEN, ETH_ALEN);
  
  parse_rreq_pdu(pkt + ETH_HLEN, &rreq_msg);

  /* Increment HopCount */
  rreq_msg.hop_count += 1;
  
  /* Do not allow loops to bring down your network */
  if (rreq_msg.hop_count > MAX_HOPCOUNT)
  {
    printf("Hop Count Exceeded %d, dropping the RREQ\n", MAX_HOPCOUNT);
    return;
  }

  /* Handle the case where the RREQ looped back to the Sender itself */
  if (strcmp(canonical_addr, rreq_msg.src_addr) == 0)
  {
    printf("Looped back RREQ received.. dropping it\n");
    return;
  }

  /* Force Discovery Set, update reverse route and flood forward */
  if (rreq_msg.force_discover)
  {
    printf("Force Discovery Flag set [RREQ] message.\n");
    del_route_entry(rreq_msg.dest_addr, &route_hash_table);
    del_route_entry(rreq_msg.src_addr, &route_hash_table);
  }

  /* Update Reverse Routes */
  route_entry = find_route_entry(rreq_msg.src_addr, &route_hash_table);

  curr_time = get_current_seconds();
  /* If no reverse route to the src, add it */
  if (route_entry == NULL)
  {
    add_route_entry(rreq_msg.src_addr, neighbor_mac, rreq_msg.hop_count, 
        local_ifindex, curr_time, rreq_msg.broadcast_id, &route_hash_table);
  }
  else
  {
    /* Honour only monotonosuly increasing broadcast IDs from a client */
    if(rreq_msg.broadcast_id >= route_entry->broadcast_id)
    {
      /* If this is a shorter route, update our route entries */
      if (rreq_msg.hop_count <= route_entry->numhops)
      {
        del_route_entry(rreq_msg.src_addr, &route_hash_table);
        add_route_entry(rreq_msg.src_addr, neighbor_mac, rreq_msg.hop_count,
            local_ifindex, curr_time, rreq_msg.broadcast_id, &route_hash_table);
      }
      else 
      {
        /* Do not flood these RREQs out, just refresh the timestamp */
        route_entry->timestamp = curr_time;
        need_forward = FALSE;
      }
    }
    else
    {
        printf("Stray RREQ with broadcast ID %d, dropping it.. \n", rreq_msg.broadcast_id);
        return;
    }
  }

  print_route_hash_table(&route_hash_table);

  if(need_forward)
  {
    rresp_msg_t * rresp = NULL;

    if (!rreq_msg.already_sent)
    {
      /* If RREQ reached the End Host, send an RRESP */
      if (strcmp(rreq_msg.dest_addr, canonical_addr) == 0)
      {
        rresp = alloc_rresp_msg(generate_broadcast_id(), rreq_msg.dest_addr, 
            rreq_msg.src_addr, 0, rreq_msg.force_discover);
      }
      else
      {
        /* At an intermediate ODR, do a route lookup and send response if appropriate */

        route_entry_t* dst_route = find_route_entry(rreq_msg.dest_addr, &route_hash_table);

        if (dst_route && ((curr_time - dst_route->timestamp) >= stale_timeout))
        {
          /* Stale Route found, remove it.. */
          printf("handle_rreq: Stale Route found for  %s, removing it..\n", rreq_msg.dest_addr);
          del_route_entry(rreq_msg.dest_addr, &route_hash_table);
          goto forward;
        }

        if (dst_route)
        {
          rresp = alloc_rresp_msg(dst_route->broadcast_id, rreq_msg.dest_addr, 
              rreq_msg.src_addr, dst_route->numhops, rreq_msg.force_discover);
          
          /* Update TimeStamp */
          route_entry->timestamp = curr_time;
        }
      }

      send_rresp(net_sockfd, local_ifindex, local_mac, neighbor_mac, rresp);
      free_rresp_msg(rresp);
      rreq_msg.already_sent = 1;
    }
forward:    
    send_rreq(net_sockfd, local_ifindex, &rreq_msg);
  }
}

void handle_rresp(char* pkt, int local_ifindex, char* canonical_addr, int net_sockfd)
{
  rresp_msg_t rresp_msg;
  route_entry_t* route_entry = NULL;
  uint32_t curr_time = 0;
  bool need_forward = TRUE;
  char neighbor_mac[ETH_ALEN];

  memcpy(neighbor_mac, pkt + ETH_ALEN, ETH_ALEN);
  
  parse_rresp_pdu(pkt + ETH_HLEN, &rresp_msg);

  /* Increment HopCount */
  rresp_msg.hop_count += 1;
  
  /* Do not allow loops to bring down your network */
  if (rresp_msg.hop_count > MAX_HOPCOUNT)
  {
    printf("handle_rresp: Hop Count Exceeded %d, dropping the RRESP\n", MAX_HOPCOUNT);
    return;
  }

  /* Force Discovery Set, update forward route (delete/add back) */
  if (rresp_msg.force_discover)
  {
    printf("Force Discovery Flag set [RREP] message.\n");
    del_route_entry(rresp_msg.src_addr, &route_hash_table);
  }
  /* Update Forward Routes */
  route_entry = find_route_entry(rresp_msg.src_addr, &route_hash_table);

  curr_time = get_current_seconds();
  /* If no forward route to the dest, add it */
  if (route_entry == NULL)
  {
    add_route_entry(rresp_msg.src_addr, neighbor_mac, rresp_msg.hop_count, 
        local_ifindex, curr_time, rresp_msg.broadcast_id, &route_hash_table);
  }
  else
  {
    /* Honour only monotonosuly increasing broadcast IDs from a client */
    if(rresp_msg.broadcast_id >= route_entry->broadcast_id)
    {
      /* If this is a shorter route, update our route entries */
      if (rresp_msg.hop_count <= route_entry->numhops)
      {
        del_route_entry(rresp_msg.src_addr, &route_hash_table);
        add_route_entry(rresp_msg.src_addr, neighbor_mac, rresp_msg.hop_count,
            local_ifindex, curr_time, rresp_msg.broadcast_id, &route_hash_table);
      }
      else 
      {
        /* Do not forward these RREPs out, just refresh the timestamp */
        route_entry->timestamp = curr_time;
        need_forward = FALSE;
      }
    }
  }

  print_route_hash_table(&route_hash_table);

  /* If RRESP reached the End Host, send MSG_PAYLOADs in the send_queue 
   * matching the destination.
   */
  if (strcmp(rresp_msg.dest_addr, canonical_addr) == 0)
  {
    printf("handle_rresp: Received response for destination %s, sending all from send queue\n", rresp_msg.src_addr);
    /* Send Pending Payloads which match the destination */
    resend_from_send_queue(&send_queue, route_entry, net_sockfd);
    return;
  }

  if(need_forward)
  {
    /* At an intermediate ODR, do a route lookup and send response if appropriate */

    route_entry_t* dst_route = find_route_entry(rresp_msg.dest_addr, &route_hash_table);

    /* If not found or found to be stale, initiate a RREQ, unlikely case */
    if (!dst_route || ((curr_time - dst_route->timestamp) >= stale_timeout))
    {
      if ( dst_route)
      {
        /* Stale Route found, remove it.. */
        printf("handle_rresp: Stale Route found for  %s, removing it..\n", rresp_msg.dest_addr);
        del_route_entry(rresp_msg.dest_addr, &route_hash_table);
      }

      /* Send RREQ */
      uint32_t broadcast_id = generate_broadcast_id();
      rreq_msg_t *rreq_msg = alloc_rreq_msg(broadcast_id, canonical_addr, 
                                      rresp_msg.dest_addr, 0, 0, rresp_msg.force_discover);
      send_rreq(net_sockfd, local_ifindex, rreq_msg);
      free_rreq_msg(rreq_msg);
      return;
    }
    else
    {
      /* Update TimeStamp */
      route_entry->timestamp = curr_time;

      /* Relay through the interface identified by the Route */
      char *local_mac = get_mac_from_ifindex(hwa_head, dst_route->ifindex);
      send_rresp(net_sockfd, dst_route->ifindex, local_mac, dst_route->nexthop, &rresp_msg);
    }
  }
}

void handle_payload(char* pkt, int local_ifindex, char* canonical_addr, int net_sockfd, int domain_sockfd)
{
  route_entry_t* route_entry = NULL;
  uint32_t curr_time = 0;
  char neighbor_mac[ETH_ALEN];
  struct sockaddr_un dst_addr;
  recv_from_odr_msg_t send_msg;
  path_port_node_t  *entry = NULL;

  memcpy(neighbor_mac, pkt + ETH_ALEN, ETH_ALEN);

  payload_msg_t * payload_msg =(payload_msg_t *)calloc(1, sizeof(payload_msg_t));
  parse_payload_pdu(pkt + ETH_HLEN, payload_msg);

  /* Increment HopCount */
  payload_msg->hop_count += 1;
  
  /* Do not allow loops to bring down your network */
  if (payload_msg->hop_count > MAX_HOPCOUNT)
  {
    printf("handle_payload: Hop Count Exceeded %d, dropping the RRESP\n", MAX_HOPCOUNT);
    free_payload_msg(payload_msg);
    return;
  }

  /* Update Forward Routes */
  route_entry = find_route_entry(payload_msg->src_addr, &route_hash_table);

  curr_time = get_current_seconds();
  /* If no forward route to the dest, add it */
  if (route_entry == NULL)
  {
    add_route_entry(payload_msg->src_addr, neighbor_mac, payload_msg->hop_count, 
        local_ifindex, curr_time, 0, &route_hash_table);
  }
  else
  {
      /* If this is a shorter route, update our route entries */
      if (payload_msg->hop_count <= route_entry->numhops)
      {
        del_route_entry(payload_msg->src_addr, &route_hash_table);
        add_route_entry(payload_msg->src_addr, neighbor_mac, payload_msg->hop_count,
            local_ifindex, curr_time, 0, &route_hash_table);
      }
      else 
      {
        /* Refresh the timestamp */
        route_entry->timestamp = curr_time;
      }
  }

  print_route_hash_table(&route_hash_table);

  /* If PAYLOAD reached the destination ODR, sent to the corresponding local process */
  if (strcmp(payload_msg->dest_addr, canonical_addr) == 0)
  {
    if ((entry = find_path_port_from_port(&path_port_list, payload_msg->dest_port)))
    {
      dst_addr.sun_family = AF_LOCAL;
      strncpy(dst_addr.sun_path, entry->path, sizeof(dst_addr.sun_path));
      memset(&send_msg, 0, sizeof(send_msg));
      build_recv_msg(&send_msg, payload_msg->src_addr, payload_msg->src_port, payload_msg->data);

      printf("[Info] : Sending (src_ip = %s, src_port=%d, msg = %s) to [dst_path = %s, dst_port = %d]\n",
          send_msg.src_addr, send_msg.src_port, send_msg.data, dst_addr.sun_path,  payload_msg->dest_port);

      if (sendto(domain_sockfd, &send_msg, sizeof(send_msg), 0, (SA*)&dst_addr, sizeof(dst_addr)) 
          != sizeof(send_msg))
        printf("[Warn]: sendto failure, check if server process is running\n");
    }
    else
    {
      printf("[Info]: Could not find Target Process, ignoring message\n");
    }
    free_payload_msg(payload_msg);
    return;
  }

  /* At an intermediate ODR, do a route lookup and forward if appropriate */

  route_entry_t* dst_route = find_route_entry(payload_msg->dest_addr, &route_hash_table);

  /* If not found or found to be stale, initiate a RREQ, unlikely case */
  if (!dst_route || ((curr_time - dst_route->timestamp) >= stale_timeout))
  {
    if (dst_route)
    {
      /* Stale Route found, remove it.. */
      printf("handle_payload: Stale Route found for  %s, removing it..\n", payload_msg->dest_addr);
      del_route_entry(payload_msg->dest_addr, &route_hash_table);
    }

    uint32_t broadcast_id = generate_broadcast_id();

    /* Add to Send Queue, we'll retransmit when RREP arrives */
    add_to_send_queue(&send_queue, payload_msg, broadcast_id);
    
    /* Send RREQ */
    rreq_msg_t *rreq_msg = alloc_rreq_msg(broadcast_id, canonical_addr, 
                                    payload_msg->dest_addr, 0, 0, 0);
    send_rreq(net_sockfd, local_ifindex, rreq_msg);
    free_rreq_msg(rreq_msg);
    
    free_payload_msg(payload_msg);
    return;
  }
  else
  {
    /* Update TimeStamp */
    route_entry->timestamp = curr_time;

    /* Forward through the interface identified by the Route */
    char *local_mac = get_mac_from_ifindex(hwa_head, dst_route->ifindex);
    send_payload(net_sockfd, dst_route->ifindex, local_mac, dst_route->nexthop, payload_msg);
    free_payload_msg(payload_msg);
  }
}

int main(int argc, char** argv)
{
  int domain_sockfd, net_sockfd, max_fd, n, nready, client_port = 0;
  struct sockaddr_un odr_addr, client_addr, dst_addr;
  socklen_t len;

  send_to_odr_msg_t   recv_msg;
  recv_from_odr_msg_t send_msg;
  fd_set all;
  char canonical_addr[IPADDR_LEN]; 	/* Canonical Addr, that of eth0 */
  char *mac;
  uint32_t curr_time = 0;

  printf("WTF\n");
  /* Verify command line parameters */
  if (argc >= 2)
  {
    stale_timeout = atoi(argv[1]);
    if (stale_timeout < 0) {
      printf("ODR Starting..\n");
      printf("Invalid staleness argument supplied, setting to default value %d seconds\n", DFLT_STALENESS);
      stale_timeout = DFLT_STALENESS;
    }
  }
  else
  {
    printf("ODR Starting with default staleness value %d seconds..\n", DFLT_STALENESS);	
    printf("Preferred usage is ./ODR_np3 <staleness>\n");
  }

  init_route_hash_table(&route_hash_table, HASH_SIZE);
  init_send_queue(&send_queue);

  hwa_head = get_hw_addrs(canonical_addr, TRUE);
  strncpy(host_name, get_host_name(canonical_addr), HOSTNAME_LEN);
  printf("Canonical IP Address :%s, Hostname :%s\n", canonical_addr, host_name);

  /* Create socket and Bind to our well known path */
  domain_sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
  printf("socket succedd\n");

  bzero(&odr_addr, sizeof(odr_addr));
  odr_addr.sun_family = AF_LOCAL;
  strcpy(odr_addr.sun_path, ODR_SUN_PATH);
  remove(ODR_SUN_PATH);

  Bind(domain_sockfd, (SA *)&odr_addr, sizeof(odr_addr));
  printf("bind succeeded\n");

  /* Initilize the Path Port List and Add the Server Well Known Path/Port */
  init_path_port_list(&path_port_list);
  add_to_path_port_list(&path_port_list, SERVER_SUN_PATH, SERVER_PORT, TTL_INFINITE);

  /* Initialize Unix datagram domain socket */
  dst_addr.sun_family = AF_LOCAL;

  /* Initialize PF_PACKET socket */
  net_sockfd = Socket(AF_PACKET, SOCK_RAW, htons(UNIQ_PROTO));

  /* Start Listening for Events */
  FD_ZERO(&all);

  while (1) {

    FD_SET(domain_sockfd, &all);
    FD_SET(net_sockfd, &all);
    max_fd = MAX(domain_sockfd, net_sockfd) + 1;

    printf("In Select: max_fd = %d\n", max_fd);
    if ((nready = select(max_fd, &all, NULL, NULL, NULL)) < 0)
    {
      if (errno == EINTR)
        continue;
      else
        err_sys("Unexpected Error in Select, quitting..\n");
    }

    if (FD_ISSET(domain_sockfd, &all))
    {
      /* Dgram Domain Socket Ready to be read */
      path_port_node_t  *entry = NULL;

      len = sizeof(client_addr);
      n = Recvfrom(domain_sockfd, &recv_msg, sizeof(recv_msg), 0, (SA*)&client_addr, &len);

      print_send_msg(&recv_msg);

      if ((entry = find_path_port_from_path(&path_port_list, client_addr.sun_path)))
      {
        client_port = entry->port;
        printf("Found Client, associated port = %d\n", client_port);
        /* Update Freshness */
        entry->ttl = get_current_seconds();
      }
      else
      {
        /* New Client, add to Path/Port List */
        client_port = generate_ephemeral_port();
        printf("New Client, associate port = %d\n", client_port);
        add_to_path_port_list(&path_port_list, client_addr.sun_path, client_port, get_current_seconds());
      }

      printf("[Info] : Received (dst_ip = %s, dst_port=%d, msg = %s) from [cli_path = %s, cli_port = %d]\n",
          recv_msg.dst_addr, recv_msg.dst_port, recv_msg.data, client_addr.sun_path, client_port);

      /* If the Client+Server on the same machine, just send it to the target 
       * process over dgram domain socket
       */

      if (0 == strcmp(recv_msg.dst_addr, canonical_addr))
      {
        if ((entry = find_path_port_from_port(&path_port_list, recv_msg.dst_port)))
        {
          strncpy(dst_addr.sun_path, entry->path, sizeof(dst_addr.sun_path));
          memset(&send_msg, 0, sizeof(send_msg));
          build_recv_msg(&send_msg, recv_msg.dst_addr, client_port, recv_msg.data);

          printf("[Info] : Sending (src_ip = %s, src_port=%d, msg = %s) to [dst_path = %s, dst_port = %d]\n",
              send_msg.src_addr, send_msg.src_port, send_msg.data, dst_addr.sun_path, recv_msg.dst_port);

          if (sendto(domain_sockfd, &send_msg, sizeof(send_msg), 0, (SA*)&dst_addr, sizeof(dst_addr)) 
              != sizeof(send_msg))
            printf("[Warn]: sendto failure, check if server process is running\n");
        }
        else
        {
          printf("[Info]: Could not find Target Process, ignoring message\n");
        }
      }
      else {
        /* Search route hash table, if present and conditions met,
         * send msg_payload, else send rreq and queue the message.
         */
        bool route_found = FALSE;
        route_entry_t* route_entry = NULL;

        payload_msg_t *payload = alloc_payload_msg(client_port, canonical_addr, recv_msg.dst_port,
            recv_msg.dst_addr, 0, recv_msg.len, recv_msg.data);

        if (recv_msg.flag)
        {
          printf("Client %s sending rreq with destination %s and force discovery flag\n", 
              host_name, get_host_name(recv_msg.dst_addr)); 
          del_route_entry(recv_msg.dst_addr, &route_hash_table);
        }

        route_entry = find_route_entry(recv_msg.dst_addr, &route_hash_table);
        if (route_entry)
        {
          curr_time = get_current_seconds();
          if ((curr_time - route_entry->timestamp) < stale_timeout) 
          {
            route_found = TRUE;
            mac = get_mac_from_ifindex(hwa_head, route_entry->ifindex);
            route_entry->timestamp = curr_time;
            send_payload(net_sockfd, route_entry->ifindex, mac, route_entry->nexthop, payload);
            free_payload_msg(payload);
          }
          else 
          {
            /* Stale Route found, remove it, initiate rreq */
            printf("Stale Route found for  %s, removing it..\n", recv_msg.dst_addr);
            del_route_entry(recv_msg.dst_addr, &route_hash_table);
          }
        }

        if (FALSE == route_found)
        {
          /* Remove any pending messages for this client, 
           * timeout should have taken care of them, just in case.. */
          delete_from_send_queue(&send_queue, payload->src_port);

          uint32_t broadcast_id = generate_broadcast_id();
          /* Add to Send Queue, we'll retransmit when RREP arrives */
          add_to_send_queue(&send_queue, payload, broadcast_id);

          /* Send RREQ */
          rreq_msg_t *rreq_msg = alloc_rreq_msg(broadcast_id, canonical_addr, 
              recv_msg.dst_addr, 0, 0, (recv_msg.flag ? 1 : 0));
          send_rreq(net_sockfd, EXCLUDE_NONE, rreq_msg);
          free_rreq_msg(rreq_msg);
        }
      }
    }
    if (FD_ISSET(net_sockfd, &all)) {
      /* Network Event, go read */
      static char pkt[ETH_FRAME_LEN];
      uint8_t msg_type;
      struct sockaddr_ll sock_addr;
      socklen_t len = sizeof(sock_addr);

      Recvfrom(net_sockfd, pkt, ETH_FRAME_LEN, 0, (SA*)&sock_addr, &len);
      msg_type = *(pkt + ETH_HLEN);
      int local_ifindex = sock_addr.sll_ifindex;

      switch(msg_type)
      {
        case MSG_RREQ: 
          handle_rreq(pkt, local_ifindex, canonical_addr, net_sockfd);
          break;
        case MSG_RRESP: 
          handle_rresp(pkt, local_ifindex, canonical_addr, net_sockfd);
          break;
        case MSG_PAYLOAD: 
          handle_payload (pkt, local_ifindex, canonical_addr, net_sockfd, domain_sockfd);
          break;
        default:
          printf("Unknown Message, Ignoring\n");
      }
    }
  }
  return 0;
}
