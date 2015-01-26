#ifndef  __DEFINES_H__
#define  __DEFINES_H__
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <sys/time.h>
#include <unp.h>

#define MAX_LINE 80
#define MAX_TTL 60
#define MAX_RESEND_LIMIT 3
#define CLIENT_TIMEOUT 10
#define RRESP_TIMEOUT 5
#define EXCLUDE_NONE (-1)
#define MAX_HOPCOUNT 64

#define ODR_SUN_PATH    "/tmp/odr.110049777"
#define SERVER_SUN_PATH "/tmp/svr.110049777"

#define IPADDR_LEN      16
#define HOSTNAME_LEN    32
#define MSG_PAYLOAD_LEN 100

#define DFLT_STALENESS  60
#define PATH_LEN        80
#define HASH_SIZE       255

#define UNIQ_PROTO 7777
#define SERVER_PORT 7777

#define TRUE    1
#define FALSE   0
typedef int bool;

//#define MAX(a,b) (((a) > (b))? (a): (b))

#endif /* __DEFINES_H__ */
