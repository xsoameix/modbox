#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <setjmp.h>
#include <netdb.h>
#include <sys/types.h>
#include <unistd.h>
#include "mtcp.h"
#include "config.h"

:class Client {

  struct {
    int sockfd;
  }
}

void
:free(self) {
  if (@sockfd != -1) close(@sockfd);
  free(self);
}

static jmp_buf fail = {0};

void
:close(self, char * err) {
  perror(err);
  longjmp(fail, 1);
}

int
:create_socket(self) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) ·close("air cilent socket failed");
  return fd;
}

addr_in_t
:build_addr(self, uint16_t port) {
  addr_in_t addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr(CLIENT_IP);
  addr.sin_port = htons(port);
  return addr;
}

void
:connect(self) {
  printf("connect socket ... ");
  fflush(stdout);
  @sockfd = ·create_socket;
  addr_in_t addr = ·build_addr(CLIENT_PORT);
  int ret = connect(@sockfd, (addr_t *) &addr, sizeof(addr_t));
  if (ret == -1) ·close("air client connect failed");
  printf("OK\n");
}

mtcp_t
:create_mtcp(self, atcp_t * atcp) {
  return (mtcp_t)      {.txn = 0, .prot = 0, .len = htons(5),
    .unit = atcp->unit, .panel = atcp->panel, .op = atcp->op,
    .addr = atcp->addr, .data = htons(atcp->data)};
}

void
:send(self, void * buf, size_t len) {
  ssize_t ret = send(@sockfd, buf, len, 0);
  if (ret == -1)       ·close("send to modbus failed");
  else if (ret != len) ·close("send to modbus mismatch");
}

void
:write(self, atcp_t * atcp) {
  //printf("sending mtcp ... ");
  //fflush(stdout);
  mtcp_t mtcp = ·create_mtcp(atcp);
  ·send(&mtcp, sizeof(mtcp));
  //printf("OK");
  //fflush(stdout);
}

#define BAUD_RATE 9600 // bits per second
#define BITS_PER_BYTE 8
#define NANOSECOND (1000 * 1000 * 1000)
#define MAX_BYTES_PER_ROUND (140 * 2) // send and recv = 1 round
#define SLEEP_TIME /* nanosecond per round */ \
  (uint64_t) MAX_BYTES_PER_ROUND * BITS_PER_BYTE * NANOSECOND / BAUD_RATE

void
:wait(self) {
  struct timespec time = {0, SLEEP_TIME}, rem;
  nanosleep(&time, &rem);
}

void
:recv(self, void * buf, size_t len) {
  ssize_t ret = recv(@sockfd, buf, len, 0);
  if (ret == 0)        ·close("modbus disconnected");
  else if (ret == -1)  ·close("recv from modbus failed");
  else if (ret != len) ·close("recv from modbus mismatch");
}

void
:print_mtcp(self, mtcp_t * mtcp) {
  printf("recv mtcp:\n"
         "  txn:   %hhX  prot:  %hhX  len:   %hhX\n"
         "  unit:  %hhX  op:    %hhX  panel: %hhX\n"
         "  addr:  %hhX  data:  %hX\n",
         mtcp->txn, mtcp->prot, mtcp->len,
         mtcp->unit, mtcp->op, mtcp->panel,
         mtcp->addr, mtcp->data);
}

#define OFFSET(structure, offset) ((uint8_t *) &structure + offset)

mtcp_t
:read(self, atcp_t * atcp) {
  mtcp_t mtcp = {0};
  size_t l, len = 0;
  l = sizeof(mhead_t), ·recv(&mtcp, l),                    len += l;
  l = ntohs(mtcp.len), ·recv(OFFSET(mtcp, len), l),        len += l;
  if (atcp->panel != ATCP_GROUP && atcp->op == ATCP_SET && mtcp.err >= 0) {
    l = sizeof(mtcp_t) - len, ·recv(OFFSET(mtcp, len), l), len += l;
  }
  //·print_mtcp(&mtcp);
  atcp->len = len - sizeof(mhead_t);
  return mtcp;
}

mtcp_t
:get_mtcp(self, atcp_t * atcp) {
  ·write(atcp);
  ·wait;
  return ·read(atcp);
}

· *
:new(jmp_buf * raise) {
  · * self = calloc(1, sizeof(·));
  @class = &Client;
  @sockfd = -1;
  memcpy(&fail, raise, sizeof(fail));
  return self;
}
