CC=gcc
#UNP_DIR=/home/raghu/np/unpv13e
UNP_DIR=/home/subin/Desktop/np/stevens/unpv13e
INCLUDE=${UNP_DIR}/lib
CFLAGS=-I${INCLUDE} -L${UNP_DIR} -g -O2 -D_REENTRANT -Wall
LIBS=-lpthread -lunp

all: client server odr

client: client.o get_hw_addrs.o local_msgs.o
	${CC} ${CFLAGS} get_hw_addrs.o local_msgs.o client.o -o client_np3 ${LIBS}

server: server.o local_msgs.o get_hw_addrs.o
	${CC} ${CFLAGS} local_msgs.o get_hw_addrs.o server.o -o server_np3 ${LIBS}

odr: odr.o path_port_mapping.o get_hw_addrs.o local_msgs.o route_hash_table.o odr_msgs.o send_queue.o 
	${CC} ${CFLAGS} path_port_mapping.o get_hw_addrs.o local_msgs.o route_hash_table.o odr_msgs.o send_queue.o odr.o -o ODR_np3 ${LIBS}

client.o: defines.h client.c
	${CC} ${CFLAGS} -c client.c

server.o: server.c
	${CC} ${CFLAGS} -c server.c

odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

local_msgs.o : local_msgs.h local_msgs.c
	${CC} ${CFLAGS} -c local_msgs.c

route_hash_table.o: route_hash_table.h route_hash_table.c
	${CC} ${CFLAGS} -c route_hash_table.c

get_hw_addrs.o : hw_addrs.h get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

odr_msgs.o : odr_msgs.h odr_msgs.c
	${CC} ${CFLAGS} -c odr_msgs.c

send_queue.o : send_queue.h send_queue.c
	${CC} ${CFLAGS} -c send_queue.c

path_port_mapping.o : path_port_mapping.h path_port_mapping.c
	${CC} ${CFLAGS} -c path_port_mapping.c
clean:
	rm -f *.o client_np3 server_np3 ODR_np3
