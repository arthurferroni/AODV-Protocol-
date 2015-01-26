#include <avr/sleep.h>
#include <ulib.h>
#include <stdio.h>
#include <stdlib.h>
#include <hal.h>
#include <aodv.h>
#include <nrk_error.h>
#include <ctype.h>
#include "my_basic_rf.h"

#define MAX_TABLE_SIZE 32
#define MAX_LIFESPAN 10
#define RREQ_BUFFER_SIZE 32

ROUTING_ENTRY routing_table[MAX_TABLE_SIZE];
AODV_RREQ_INFO rreq_buffer[RREQ_BUFFER_SIZE];

uint8_t node_addr;
uint8_t node_seq_num = 1;
uint8_t dest_seq_num = 0;

nrk_time_t timeout_t;
uint8_t timeout = 5;

uint8_t aodv_id;
uint8_t table_size = 0;
uint8_t rreq_buffer_size = 0;

RF_TX_INFO rfTxInfo;
RF_RX_INFO rfRxInfo;
uint8_t tx_buf[RF_MAX_PAYLOAD_SIZE];
uint8_t rx_buf[RF_MAX_PAYLOAD_SIZE];


void renew_routing_entry(uint8_t dest, uint8_t dest_seq_num) {
  uint8_t index = find_index(dest, dest_seq_num);
  if (index > -1) {
    routing_table[index].lifespan = MAX_LIFESPAN;
  }
}

int8_t delete_routing_entry(uint8_t dest, uint8_t next_hop) {
  int i;
  for (i = 0; i < MAX_TABLE_SIZE; i++) {
    if (routing_table[i].dest == dest && routing_table[i].next_hop == next_hop) {
      for (i; i < MAX_TABLE_SIZE-1; i++) {
        routing_table[i] = routing_table[i+1];
      }
      table_size--;
      break;
    }
  }
  if (i == MAX_TABLE_SIZE) return -1;
  return 0;
}

int8_t add_routing_entry(uint8_t dest, uint8_t next_hop, uint8_t dest_seq_num, uint8_t hop_count, int8_t snr){
  if(table_size < MAX_TABLE_SIZE){
    int i;
    for (i=0; i<table_size; i++) { 
      if (routing_table[i].dest == dest && routing_table[i].next_hop == next_hop) {
        if (routing_table[i].dest_seq_num < dest_seq_num) {
          routing_table[i].dest = dest;
          routing_table[i].next_hop = next_hop;
          routing_table[i].dest_seq_num = dest_seq_num;
          routing_table[i].hop_count = hop_count;
          routing_table[i].ssnr2 = 0.5 * (routing_table[i].ssnr2 + snr);
          return 1; // updating
        } else {
          return -1;
        }
      }
    }
    routing_table[table_size].dest = dest;
    routing_table[table_size].next_hop = next_hop;
    routing_table[table_size].dest_seq_num = dest_seq_num;
    routing_table[table_size].hop_count = hop_count;
    routing_table[table_size].ssnr2 = 0.5 * (routing_table[table_size].ssnr2 + snr);
    table_size++;
    return 0; // adding new entry
  }
  return -1;
}

int8_t clean_routing_table() {
  table_size = 0;
  return 0;
}

void print_routing_table(){
  uint8_t i;
  for(i=0 ; i<table_size ; i++){
    printf("[DEBUG-routing-entry] dest: %d | next_hop: %d | hop_count: %d | dest_seq_num: %d | neighbor_len: %d | lifespan: %d | ssnr2: %d\r\n", 
      routing_table[i].dest, routing_table[i].next_hop, routing_table[i].hop_count, 
      routing_table[i].dest_seq_num, routing_table[i].neighbor_len, routing_table[i].lifespan,
      routing_table[i].ssnr2);
  }
}

uint8_t get_msg_type(uint8_t* rx_buf){
  return rx_buf[0];
}

void unpack_aodv_rrep(uint8_t* rx_buf, AODV_RREP_INFO* aodvrrep){
  aodvrrep->type = rx_buf[0];
  aodvrrep->src = rx_buf[1];
  aodvrrep->dest = rx_buf[2];
  aodvrrep->dest_seq_num = rx_buf[3];
  aodvrrep->hop_count = rx_buf[4];
  aodvrrep->lifespan = rx_buf[5];
}

uint8_t pack_aodv_rrep(uint8_t* tx_buf, AODV_RREP_INFO aodvrrep){
  tx_buf[0] = aodvrrep.type;
  tx_buf[1] = aodvrrep.src;
  tx_buf[2] = aodvrrep.dest;
  tx_buf[3] = aodvrrep.dest_seq_num;
  tx_buf[4] = aodvrrep.hop_count;
  tx_buf[5] = aodvrrep.lifespan;
  return 6;
}

void unpack_aodv_rreq(uint8_t* rx_buf, AODV_RREQ_INFO* aodvrreq){
  aodvrreq->type = rx_buf[0];
  aodvrreq->broadcast_id = rx_buf[1];
  aodvrreq->src = rx_buf[2];
  aodvrreq->src_seq_num = rx_buf[3];
  aodvrreq->dest = rx_buf[4];
  aodvrreq->dest_seq_num = rx_buf[5];
  aodvrreq->sender_addr = rx_buf[6];
  aodvrreq->hop_count = rx_buf[7];
}

uint8_t pack_aodv_rreq(uint8_t* tx_buf, AODV_RREQ_INFO aodvrreq){
  tx_buf[0] = aodvrreq.type;
  tx_buf[1] = aodvrreq.broadcast_id;
  tx_buf[2] = aodvrreq.src;
  tx_buf[3] = aodvrreq.src_seq_num;
  tx_buf[4] = aodvrreq.dest;
  tx_buf[5] = aodvrreq.dest_seq_num;
  tx_buf[6] = aodvrreq.sender_addr;
  tx_buf[7] = aodvrreq.hop_count;
  return 8;
}

void unpack_aodv_msg(uint8_t* rx_buf, AODV_MSG_INFO* aodvmsg, uint8_t* msg){
  aodvmsg->type = rx_buf[0];
  aodvmsg->src  = rx_buf[1];
  aodvmsg->next_hop = rx_buf[2];
  aodvmsg->dest = rx_buf[3];
  aodvmsg->msg_seq_no = tx_buf[4];
  aodvmsg->msg_len = rx_buf[5];
  aodvmsg->msg = msg;
  memcpy(msg, rx_buf+6, aodvmsg->msg_len);
}

uint8_t pack_aodv_msg(uint8_t* tx_buf, AODV_MSG_INFO aodvmsg){
  tx_buf[0] = aodvmsg.type;
  tx_buf[1] = aodvmsg.src;
  tx_buf[2] = aodvmsg.next_hop;
  tx_buf[3] = aodvmsg.dest;
  tx_buf[4] = aodvmsg.msg_seq_no;
  tx_buf[5] = aodvmsg.msg_len;
  memcpy(tx_buf+6, aodvmsg.msg, aodvmsg.msg_len);
  return 6+aodvmsg.msg_len;
}

void unpack_aodv_rerr(uint8_t* rx_buf, AODV_RERR_INFO* aodvrerr){
  aodvrerr->type = rx_buf[0];
}

uint8_t pack_aodv_rerr(uint8_t* tx_buf, AODV_RERR_INFO aodvrerr){
  tx_buf[0] = aodvrerr.type;
  return 1;
}

void repack_forward_msg(AODV_MSG_INFO* aodvmsg, uint8_t next_hop){
  aodvmsg->next_hop = next_hop;
}

uint8_t find_index(uint8_t dest, uint8_t dest_seq_num){
  uint8_t i;
  for(i = 0; i < table_size; i++){
    if(routing_table[i].dest == dest && routing_table[i].dest_seq_num == dest_seq_num)
      return i;
  }
  return -1; // did not find in routing table
}

uint8_t find_next_hop(uint8_t dest){
  uint8_t i;
  for(i = 0; i < table_size ; i++){
    if(routing_table[i].dest == dest){
      return routing_table[i].next_hop;
    }
  }
  return 0; // 0 => did not find in routing table
}

uint8_t find_next_hop_by_ssnr2(uint8_t dest){
  uint8_t i;
  uint8_t dest_len = 0;
  uint8_t dests[MAX_TABLE_SIZE];
  int ssnr = -1;
  int entry = -1;
  for(i = 0; i < table_size ; i++){
    if(routing_table[i].dest == dest){
      dests[dest_len++] = i;
    }
  }
  for (i = 0; i < dest_len; i++) {
    if (routing_table[dests[i]].ssnr2 > ssnr) {
      entry = dests[i];
      ssnr = routing_table[entry].ssnr2;
    }
  }
  if (entry == -1)
    return 0;
  else
    return routing_table[entry].next_hop; // 0 => did not find in routing table
}

uint8_t find_next_hop_by_ssnr2_and_hop_count(uint8_t dest) {
  uint8_t i;
  uint8_t dest_len = 0;
  uint8_t dests[MAX_TABLE_SIZE];
  int ssnr = -1;
  int hop_count = -1;
  int entry = -1;
  for(i = 0; i < table_size ; i++){
    if(routing_table[i].dest == dest){
      dests[dest_len++] = i;
    }
  }
  for (i = 0; i < dest_len; i++) {
    if (routing_table[dests[i]].ssnr2 > ssnr) {
      entry = dests[i];
      ssnr = routing_table[entry].ssnr2;
    }
    if (routing_table[dests[i]].ssnr2 == ssnr) {
      if (routing_table[dests[i]].hop_count < hop_count) {
        entry = dests[i];
        hop_count = routing_table[entry].hop_count;
      }
    }
  }
  if (entry == -1)
    return 0;
  else
    return routing_table[entry].next_hop; // 0 => did not find in routing table
}

uint8_t broadcast_rreq(uint8_t *tx_buf, uint8_t length) {
  rfTxInfo.pPayload = tx_buf;
  rfTxInfo.length = length+5;
  rfTxInfo.destAddr = 0xffff; // broadcast by default
  rfTxInfo.cca = 0;
  rfTxInfo.ackRequest = 1;

  // printf( "Sending\r\n" );
  return rf_tx_packet(&rfTxInfo);

  /*
  if(rf_tx_packet(&rfTxInfo) != 1){
    nrk_kprintf (PSTR ("@@@ RF_TX ERROR @@@\r\n"));
    nrk_wait(timeout_t);
  }
  else
    nrk_kprintf (PSTR ("=== Tx task sent data! ===\r\n"));
    */
}

uint8_t send_packet(uint8_t *tx_buf, uint8_t length){
  rfTxInfo.pPayload = tx_buf;
  rfTxInfo.length = length+5;
  rfTxInfo.destAddr = tx_buf[2]; // next_hop
  rfTxInfo.cca = 0;
  rfTxInfo.ackRequest = 1;

  return rf_tx_packet(&rfTxInfo);
  /*
  if(rf_tx_packet(&rfTxInfo) != 1){
    nrk_kprintf (PSTR ("@@@ RF_TX ERROR @@@\r\n"));
    nrk_wait(timeout_t);
  }
  else
    nrk_kprintf (PSTR ("=== Tx task sent data! ===\r\n"));
    */
}

uint8_t send_rrep(uint8_t *tx_buf, uint8_t next_hop, uint8_t length){
  rfTxInfo.pPayload = tx_buf;
  rfTxInfo.length = length+5;
  rfTxInfo.destAddr = next_hop;
  rfTxInfo.cca = 0;
  rfTxInfo.ackRequest = 1;

  return rf_tx_packet(&rfTxInfo);

  /*
  if(rf_tx_packet(&rfTxInfo) != 1){
    nrk_kprintf (PSTR ("@@@ RF_TX ERROR @@@\r\n"));
    nrk_wait(timeout_t);
  }
  else
    nrk_kprintf (PSTR ("=== Tx task sent data! ===\r\n"));
    */
}

uint8_t send_rerr(uint8_t *tx_buf, uint8_t next_hop, uint8_t length){
  rfTxInfo.pPayload = tx_buf;
  rfTxInfo.length = length+5;
  rfTxInfo.destAddr = next_hop;
  rfTxInfo.cca = 0;
  rfTxInfo.ackRequest = 1;

  return rf_tx_packet(&rfTxInfo);

  /*
  if(rf_tx_packet(&rfTxInfo) != 1){
    nrk_kprintf (PSTR ("@@@ RF_TX ERROR @@@\r\n"));
    nrk_wait(timeout_t);
  }
  else
    nrk_kprintf (PSTR ("=== Tx task sent data! ===\r\n"));
    */
}

int8_t add_rreq_to_buffer(AODV_RREQ_INFO* aodvrreq) {
  if(rreq_buffer_size < RREQ_BUFFER_SIZE){
    int i;
    for (i=0; i<table_size; i++) { 
      if ((rreq_buffer[i].src == aodvrreq->src) && (rreq_buffer[i].sender_addr == aodvrreq->sender_addr)) {
        if (rreq_buffer[i].broadcast_id < aodvrreq->broadcast_id) {
          rreq_buffer[i].type = aodvrreq->type;
          rreq_buffer[i].broadcast_id = aodvrreq->broadcast_id;
          rreq_buffer[i].src = aodvrreq->src;
          rreq_buffer[i].src_seq_num = aodvrreq->src_seq_num;
          rreq_buffer[i].dest = aodvrreq->dest;
          rreq_buffer[i].dest_seq_num = aodvrreq->dest_seq_num;
          rreq_buffer[i].sender_addr = aodvrreq->sender_addr;
          rreq_buffer[i].hop_count = aodvrreq->hop_count;
          return 0; // updating
        } else {
          return -1;
        }
      }
    }
    rreq_buffer[rreq_buffer_size].type = aodvrreq->type;
    rreq_buffer[rreq_buffer_size].broadcast_id = aodvrreq->broadcast_id;
    rreq_buffer[rreq_buffer_size].src = aodvrreq->src;
    rreq_buffer[rreq_buffer_size].src_seq_num = aodvrreq->src_seq_num;
    rreq_buffer[rreq_buffer_size].dest = aodvrreq->dest;
    rreq_buffer[rreq_buffer_size].dest_seq_num = aodvrreq->dest_seq_num;
    rreq_buffer[rreq_buffer_size].sender_addr = aodvrreq->sender_addr;
    rreq_buffer[rreq_buffer_size].hop_count = aodvrreq->hop_count;
    rreq_buffer_size++;
    return 0; // adding new entry
  } else {
    printf("ERROR: RREQ_BUFFER_SIZE exceeded!!!");
  }
  return -1;
}

int8_t check_rreq_is_valid(AODV_RREQ_INFO* aodvrreq) {
  // check node received a RREQ with the same broadcast_id & source addr
  // if it did, return -1 (drop the rreq packet); return 0, otherwise.
  if (node_addr == aodvrreq->src) {
    printf("rejecting in first cond...\r\n");
    return -1;
  }   
  
  if (rreq_buffer_size == 0) {
    // buffer it, if it is a valid rreq.
    add_rreq_to_buffer(aodvrreq);
    printf("first return 0\r\n");
    return 0;
  } else {
    int i;
    for (i=0; i<rreq_buffer_size; i++) {
      if ((rreq_buffer[i].src == aodvrreq->src) && (rreq_buffer[i].sender_addr == aodvrreq->sender_addr)) {
        printf("rreq_buffer[%d].broadcast_id = %d  ||  aodvrreq->broadcast_id = %d ...\r\n", i, rreq_buffer[i].broadcast_id, aodvrreq->broadcast_id);
        if (rreq_buffer[i].broadcast_id >= aodvrreq->broadcast_id) {
          printf("rejecting in second cond...\r\n");
          return -1; // reject
        } else {
          add_rreq_to_buffer(aodvrreq); // update
          printf("second return 0\r\n");
          return 0;
        }
      } 
    }

    // completely new rreq
    // buffer it, if it is a valid rreq.
    add_rreq_to_buffer(aodvrreq);
    printf("third return 0\r\n");
    return 0;
  }
}

void print_rreq_buffer() {
  int i;
  for (i=0; i<rreq_buffer_size; i++) {
    printf("[DEBUG-rreq-buffer] type: %d | broadcast_id: %d | src: %d | src_seq_num: %d | dest: %d | dest_seq_num: %d | sender_addr: %d | hop_count: %d\r\n", 
      rreq_buffer[i].type, rreq_buffer[i].broadcast_id, rreq_buffer[i].src, 
      rreq_buffer[i].src_seq_num, rreq_buffer[i].dest, rreq_buffer[i].dest_seq_num,
      rreq_buffer[i].sender_addr, rreq_buffer[i].hop_count);
  }
}
