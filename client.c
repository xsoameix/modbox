#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <time.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include "mtcp.h"

:require "common.c"

:class Client {

  struct {
    addr_info_t * addr;
    int sockfd;
  }
}

void
:free(self) {
  if (@sockfd != -1) close(@sockfd);
  if (@addr) freeaddrinfo(@addr);
  free(self);
}

static jmp_buf fail = {0};

void
:close(self, char * err) {
  perror(err);
  longjmp(fail, 1);
}

addr_info_t *
:build_addr(self) {
  addr_info_t hint = {0}, * res;
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  getaddrinfo(Common.fetch_env_addr("CONNECT_ADDR", "127.0.0.1"),
              Common.fetch_env_port("CONNECT_PORT", "60000"), &hint, &res);
  return res;
}

int
:create_socket(self) {
  int fd = socket(@addr->ai_family, @addr->ai_socktype, @addr->ai_protocol);
  if (fd == -1) ·close("air cilent socket failed");
  return fd;
}

void
:connect(self) {
  @addr = ·build_addr;
  @sockfd = ·create_socket;
  int ret = connect(@sockfd, @addr->ai_addr, @addr->ai_addrlen);
  if (ret == -1) ·close("air client connect failed");
  puts("modbus connected");
  fflush(stdout);
}

void
:print_atcp(self, atcp_t * atcp) {
  printf("send unit:  %hhX  op:  %hhX  panel: %hhX  addr:  %hhX  data:  %hX\n",
         atcp->unit, atcp->op, atcp->panel, atcp->addr, atcp->data);
  fflush(stdout);
}

void
:create_mtcp(self, mtcp_t * mtcp, atcp_t * atcp) {
  mhead_in_t head = {.txn = 0, .prot = 0, .len = htons(sizeof(head))};
  * mtcp = (mtcp_t) {
    .unit = atcp->unit, .panel = atcp->panel, .op = atcp->op,
    .addr = atcp->addr, .data = htons(atcp->data)};
  memcpy(&mtcp->head, &head, sizeof(head));
}

void
:send(self, void * buf, size_t len) {
  ssize_t ret = send(@sockfd, buf, len, 0);
  if (ret == -1)       ·close("send to modbus failed");
  else if (ret != len) ·close("send to modbus mismatch");
}

void
:write(self, atcp_t * atcp) {
  ·print_atcp(atcp);
  mtcp_t mtcp = {0};
  ·create_mtcp(&mtcp, atcp);
  ·send(&mtcp, sizeof(mtcp));
}

#define BAUD_RATE 9600 // bits per second
#define BITS_PER_BYTE 8
#define NANOSECOND (1000 * 1000 * 1000)
#define MAX_BYTES_PER_ROUND (140 * 2) // send and recv = 1 round
#define SLEEP_TIME /* nanosecond per round */ \
  (uint64_t) MAX_BYTES_PER_ROUND * BITS_PER_BYTE * NANOSECOND / BAUD_RATE

void
:wait(self, atcp_t * atcp) {
  time_t sec = atcp->op == ATCP_GET ? 0 : 0;
  struct timespec time = {sec, SLEEP_TIME}, rem;
  nanosleep(&time, &rem);
}

ssize_t
:pull(self, void * buf, size_t len) {
  ssize_t ret = -1;
  ssize_t i = 0;
  for (; i < len; ) {
    ret = recv(@sockfd, buf, len - i, MSG_DONTWAIT);
    printf("ret %zd len %zu", ret, len);
    int j = 0;
    for (; j < ret; j++) {
      printf(" %02X", ((uint8_t *) buf)[j]);
    }
    putchar('\n');
    fflush(stdout);
    if (ret <= 0) {
      return ret;
    } else {
      i += ret;
    }
  }
  return ret;
}

int
:recv(self, void * buf, size_t len) {
  ssize_t ret = ·pull(buf, len);
  if (ret == 0) {
    ·close("modbus disconnected");
  } else if (ret == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
    ·close("recv from modbus failed");
  }
  //else if (ret != len) ·close("recv from modbus mismatch");
}

#define GLEN sizeof(mget_t) // get len
#define SLEN sizeof(mset_t) // set len

size_t
:read(self, mbuf_t * buf, atcp_t * atcp) {
  ·recv(buf, GLEN);
  mget_t * get = (mget_t *) buf;
  if (get->err < 0) return GLEN;
  if (atcp->op == ATCP_GET) {
    ·recv(OFFSET(buf, GLEN), get->size);
    return GLEN + get->size;
  } else {
    ·recv(OFFSET(buf, GLEN), SLEN - GLEN);
    return SLEN;
  }
}

size_t
:get_mtcp(self, mbuf_t * buf, atcp_t * atcp) {
  ·write(atcp);
  ·wait(atcp);
  return ·read(buf, atcp);
}

· *
:new(jmp_buf * raise) {
  · * self = calloc(1, sizeof(·));
  @class = &Client;
  @addr = 0;
  @sockfd = -1;
  memcpy(&fail, raise, sizeof(fail));
  return self;
}
