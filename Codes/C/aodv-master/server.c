#include "defines.h"
#include "local_msgs.h"
#include "unp.h"
#include "hw_addrs.h"
#include <stdio.h>

static inline char* get_current_time()
{
  static char buf[48];
  time_t ticks;
  ticks = time(NULL);
  snprintf(buf, sizeof(buf), "%.24s", ctime(&ticks));
  return buf;
}

int main(int argc, char** argv)
{
  char recv_msg[MSG_PAYLOAD_LEN], client_ip[IPADDR_LEN], host_name[PATH_LEN], canonical_addr[IPADDR_LEN];
  int sock_fd, client_port;
  struct sockaddr_un srv_addr;
  socklen_t srv_addrLen = sizeof(srv_addr);


  struct hwa_info *hwa_head = get_hw_addrs(canonical_addr, TRUE);
  strncpy(host_name, get_host_name(canonical_addr), HOSTNAME_LEN);
  printf("Canonical IP Address :%s, Hostname :%s\n", canonical_addr, host_name);

  sock_fd = Socket(AF_LOCAL, SOCK_DGRAM, 0);

  bzero(&srv_addr, sizeof(srv_addr));
  srv_addr.sun_family = AF_LOCAL;
  strcpy(srv_addr.sun_path, SERVER_SUN_PATH);
  remove(SERVER_SUN_PATH);

  Bind(sock_fd, (SA*)&srv_addr, srv_addrLen);

  while (1)
  {
    msg_recv(sock_fd, recv_msg, client_ip, &client_port);
    printf("Server at node %s responding to request [%s] from %s [%s:%d]\n", 
        host_name, recv_msg, get_host_name(client_ip), client_ip, client_port);
    msg_send(sock_fd, client_ip, client_port, get_current_time(), 0);
  }

  return 0;
}
