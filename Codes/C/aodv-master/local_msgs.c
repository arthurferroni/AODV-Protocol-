#include "local_msgs.h"
#include "defines.h"
#include "unp.h"

void build_send_msg(send_to_odr_msg_t *msg, char* dst_addr, int dst_port, char* data, int flag)
{
  assert(msg);

  strncpy((char *)&(msg->dst_addr), dst_addr, IPADDR_LEN);

  msg->dst_port = dst_port;
  msg->flag = flag;

  msg->len = strlen(data)+1;
  strncpy((char *)&(msg->data), data, MSG_PAYLOAD_LEN);
}

void print_send_msg(send_to_odr_msg_t *msg)
{
  printf("[Info] Sent msg to ODR:\n");
  printf("\tDest Addr:%s\n\tDest Port:%d\n\tFlag:%d\n\tMsg:%s\n",
      msg->dst_addr, msg->dst_port, msg->flag, msg->data);
}

void build_recv_msg(recv_from_odr_msg_t *msg, char* src_addr, int src_port, char* data)
{
  assert(msg);

  strncpy((char *)&(msg->src_addr), src_addr, IPADDR_LEN);

  msg->src_port = src_port;

  msg->len = strlen(data)+1;
  strncpy((char *)&(msg->data), data, MSG_PAYLOAD_LEN);
}

void print_recv_msg(recv_from_odr_msg_t *msg)
{
  printf("[Info] Received msg from ODR:\n");
  printf("\tSrc Addr:%s\n\tSrc Port:%d\n\tMsg:%s\n", 
      msg->src_addr, msg->src_port, msg->data);
}

/* Routine to send a request from the client to the local ODR server */
void msg_send(int sock_fd, char* dst_addr, int dst_port, char* data, int flag)
{ 
  struct sockaddr_un odr_addr;
  send_to_odr_msg_t send_msg;

  odr_addr.sun_family = AF_LOCAL;
  strcpy(odr_addr.sun_path, ODR_SUN_PATH);    

  bzero(&send_msg, sizeof(send_to_odr_msg_t));
  build_send_msg(&send_msg, dst_addr, dst_port, data, flag);
  print_send_msg(&send_msg);
  Sendto(sock_fd, &send_msg, sizeof(send_to_odr_msg_t), 0, (SA*)&odr_addr, sizeof(odr_addr));
}

/* Routine to receive a response from the local ODR server */
void msg_recv(int sock_fd, char* msg, char* src_addr, int* src_port) 
{
  struct sockaddr_un odr_addr;
  socklen_t len = sizeof(odr_addr);
  recv_from_odr_msg_t recv_msg;
  int n;

  while ((n = Recvfrom(sock_fd, (void *)(&recv_msg), sizeof(recv_from_odr_msg_t), 0, (SA*)&odr_addr, &len)) < 0)
  {
    if (EINTR == errno)
      continue;
    else
      err_sys("msg_recv interrupted\n");
  }

  strncpy(msg, recv_msg.data, recv_msg.len);
  if (src_addr)
  {
    strncpy(src_addr, recv_msg.src_addr, IPADDR_LEN);
  }
  if (src_port)
  {
    *src_port = recv_msg.src_port;
  }
}
