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

· *
:new(void) {
  · * self = calloc(1, sizeof(·));
  @class = &Client;
  @sockfd = -1;
  return self;
}

void
:free(self) {
  if (@sockfd != -1) {
    close(@sockfd);
  }
  free(self);
}

static jmp_buf fail = {0};

int
:create_socket(void) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd == -1) {
    perror("socket failed");
    longjmp(fail, 1);
  }
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
  @sockfd = Client.create_socket();
  addr_in_t addr = ·build_addr(CLIENT_PORT);
  int ret = connect(@sockfd, (addr_t *) &addr,
                    sizeof(addr_t));
  if (ret == -1) {
    perror("connect failed");
    longjmp(fail, 1);
  }
  printf("OK\n");
}

mtcp_t
:mtcp_create(atcp_t * atcp) {
  return (mtcp_t) {
    .txn = 0, .prot = 0, .len = htons(5),
    .unit = atcp->unit, .panel = atcp->panel, .op = atcp->op,
    .addr = atcp->addr, .data = htons(atcp->data)
  };
}

void
:send(self, atcp_t * atcp) {
  printf("sending mtcp ... ");
  fflush(stdout);
  mtcp_t mtcp = Client.mtcp_create(atcp);
  ssize_t ret = write(@sockfd, &mtcp, sizeof(mtcp));
  if (ret == 0) {
    perror("air server disconnected");
    longjmp(fail, 1);
  } else if (ret == -1) {
    perror("send to air server failed");
    longjmp(fail, 1);
  }
  printf("OK");
  fflush(stdout);
}

void
:print_mtcp(mtcp_t * mtcp, uint8_t readlen) {
  if (readlen == sizeof(mtcp)) {
    printf("recv mtcp:\n"
           "  txn:   %hhX  prot:  %hhX  len:   %hhX\n"
           "  unit:  %hhX  op:    %hhX  panel: %hhX\n"
           "  addr:  %hhX  data:  %hX\n",
           mtcp->txn, mtcp->prot, mtcp->len,
           mtcp->unit, mtcp->op, mtcp->panel,
           mtcp->addr, mtcp->data);
  } else {
    uint8_t i = 0;
    for (; i < readlen; i++) {
      printf("%02X ", ((uint8_t *) mtcp)[i]);
    }
    puts("");
  }
}

mtcp_t
:recv(self, uint8_t readlen) {
  mtcp_t mtcp = {0};
  ssize_t ret = read(@sockfd, &mtcp, readlen);
  if (ret == 0) {
    perror("air server disconnected");
    longjmp(fail, 1);
  } else if (ret == -1) {
    perror("recv from air server failed");
    longjmp(fail, 1);
  }
  Client.print_mtcp(&mtcp, readlen);
  return mtcp;
}

· *
:create(jmp_buf * raise) {
  memcpy(&fail, raise, sizeof(fail));
  return Client.new();
}
