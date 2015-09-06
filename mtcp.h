#ifndef MTCP_H
#define MTCP_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct sockaddr_in6     addr_in6_t;
typedef struct sockaddr_in      addr_in_t;
typedef struct sockaddr         addr_t;
typedef struct sockaddr_storage addr_store_t;
typedef struct addrinfo         addr_info_t;

typedef struct { // modbus application protocol (MBAP) header without unit id
  uint16_t txn;   // transaction id
  uint16_t proto; // protocol
  uint16_t len;
} mph_t;

typedef struct list {
  struct node * head;
  struct node * tail;
} list_t;

typedef struct node {
  struct node * prev;
  struct node * next;
} node_t;

typedef struct {
  list_t run;
  list_t ready;
  list_t sleep;
  pthread_mutex_t run_mutex;
  pthread_mutex_t sleep_mutex;
  pthread_cond_t  sleep_cond;
  pthread_t cleaner;
  int cleanup;
  int finished;
} queue_t;

#endif
