#include <unp.h>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include "defines.h"
#include "local_msgs.h"
#include "hw_addrs.h"

static sigjmp_buf jmpbuf;

void sig_alarm(int signo)
{
	siglongjmp(jmpbuf, 1);
}

char *getserverip(char *hostname)
{
  static char hostAddr[IPADDR_LEN];
  struct hostent * hptr; 
  char **pptr; 
  if ((hptr = gethostbyname(hostname)))
  {
    assert(AF_INET == hptr->h_addrtype);
    pptr = hptr -> h_addr_list;
    assert(*pptr);
    Inet_ntop(AF_INET, *pptr, hostAddr, IPADDR_LEN);
    return hostAddr;
  }
  return NULL;
}

int main()
{
	int sockfd, redo=0;
	char tempFile[]="/tmp/110009168.XXXXXX";
	char server_vm[MAX_LINE],hostname[MAX_LINE],first_msg[]="time please!";
	struct hostent *he;
	struct sockaddr_un cliaddr;
	char recvmsg[MSG_PAYLOAD_LEN];
	char msg_source_ip[IPADDR_LEN], *server_ip = NULL;
	int msg_source_port;
	char canon_addr[IPADDR_LEN]; 	/* Canonical Addr, that of eth0 */

        struct hwa_info *hwa_head = get_hw_addrs(canon_addr, TRUE);
	printf("Canonical IP Address :%s\n", canon_addr);
	
        if( mkstemp(tempFile) == -1)
	{
		err_quit("error creating temp file");
	}

        remove(tempFile);
	sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0);
	bzero(&cliaddr, sizeof(cliaddr));
	cliaddr.sun_family = AF_LOCAL;
	strncpy(cliaddr.sun_path, tempFile, sizeof(cliaddr.sun_path));
	Bind(sockfd, (SA*)&cliaddr, sizeof(cliaddr));
	// TO DO: find the hostname ..	
	/*	if(( he = gethostbyaddr(&cliaddr.sin_addr.s_addr, sizeof cliaddr.sin_addr.s_addr, AF_INET)) == NULL){
			herror("gethostbyaddr");
	 		return 2;
	 	}
		strcmp(hostname,he->h_name);
		*/
	strcpy(hostname,"pending");

	Signal(SIGALRM, sig_alarm);
	while(1)
	{
                redo = 0;
		printf(" Please select the server node: [vm1, vm2, vm3, ....vm10], or type <quit> to exit:\n");
		scanf("%s",&server_vm);
		if(!strcmp(server_vm,"quit"))
			exit(0);
		
		if((strcmp(server_vm,"vm1") && strcmp(server_vm,"vm2") && strcmp(server_vm,"vm3") && \
                      strcmp(server_vm,"vm4") && strcmp(server_vm,"vm5") && strcmp(server_vm,"vm6") && \
                      strcmp(server_vm,"vm7") && strcmp(server_vm,"vm8") && strcmp(server_vm,"vm9") && \
                      strcmp(server_vm,"vm10")))
		{
			printf("Invalid input\n");
			continue;
		}

		printf("client at '%s' sending request to server at %s...\n", hostname, server_vm);
	
                //server_ip = "127.0.0.1"; /* TODO: Remove this */
                if (!(server_ip = getserverip(server_vm)))
                {
                    printf("Couldn't find the IP corresponding to server_vm %s\n", server_vm);
                    continue;
                }
                printf("Server IP is %s\n", server_ip);

sendagain:	msg_send(sockfd, server_ip, SERVER_PORT, first_msg, redo? 1: 0);
		alarm(CLIENT_TIMEOUT);
		if(sigsetjmp(jmpbuf, 1) != 0)
		{
			if(redo >= MAX_RESEND_LIMIT)
			{
				printf("maximum resend limit(%d) exceeded from client %s to server %s\n", redo, hostname, server_vm);
				continue;
			}
			
			printf(" time out occured, client %s resending request to server %s\n", hostname, server_vm);
			redo += 1;
			goto sendagain;
		} 

		msg_recv(sockfd, recvmsg, msg_source_ip, &msg_source_port);
		printf("Message recieved from server %s at client %s: %s\n", server_vm, hostname, recvmsg);
		alarm(0);
	}

	unlink(tempFile);
	return 0;
}	
