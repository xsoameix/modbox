#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "mtcp.h"
#include "config.h"

:require "client.c"

:class Worker {

  struct {
    int sockfd;
    jmp_buf * fail;
  }
}

· *
:new(int * fd, jmp_buf * fail) {
  · * self = calloc(1, sizeof(·));
  @class = &Worker;
  @sockfd = * fd;
  @fail = fail;
  return self;
}

void
:free(self) {
  close(@sockfd);
  free(self);
}

int
:read_atcp(self, atcp_t * atcp) {
  ssize_t ret = read(@sockfd, atcp, sizeof(atcp));
  if (ret == 0) {
    perror("client disconnected");
    return -1;
  } else if (ret == -1) {
    perror("read failed");
    return -1;
  }
  atcp->data = ntohs(atcp->data);
}

atcp_t
:atcp_create(atcp_t * atcp, mtcp_t * recv) {
  return (atcp_t) {
    .close = atcp->close, .readlen = atcp->readlen,
    .unit = recv->unit, .panel = recv->panel, .op = recv->op,
    .addr = recv->addr, .data = recv->data
  };
}

void
:send(self, atcp_t * atcp, mtcp_t * recv) {
  atcp_t send = Worker.atcp_create(atcp, recv);
  ssize_t ret = write(@sockfd, &send, sizeof(send));
  if (ret == 0) {
    perror("air server disconnected");
    longjmp(* @fail, 1);
  } else if (ret == -1) {
    perror("send to air server failed");
    longjmp(* @fail, 1);
  }
}

void
:print_atcp(atcp_t * atcp) {
  printf("recv atcp:\n"
         "  unit:  %hhX  op:    %hhX  panel: %hhX"
         "  addr:  %hhX  data:  %hX\n",
         atcp->unit, atcp->op, atcp->panel,
         atcp->addr, atcp->data);
}

int
:recv(self, client_t * client) {
  atcp_t atcp = {0};
  int ret = ·read_atcp(&atcp);
  if (ret == -1) return 0;
  Worker.print_atcp(&atcp);
  //client·send(&atcp);
  //mtcp_t mtcp = client·recv(atcp.readlen);
  mtcp_t mtcp = Client.mtcp_create(&atcp);
  ·send(&atcp, &mtcp);
  if (atcp.close) return 0;
  return 1;
}

void *
:run(void * fd) {
  jmp_buf fail = {0};
  · * self = Worker.new(fd, &fail);
  client_t * client = 0;
  int failed = setjmp(fail);
  if (!failed) {
    client = Client.create(&fail);
    //client·connect;
    while (·recv(client));
  }
  client·free;
  ·free;
  pthread_detach(pthread_self());
}

int
:thread(int fd) {
  pthread_t thread;
  int create = pthread_create(&thread, 0, Worker.run, &fd);
  if (create == -1) {
    perror("could not cerate thread");
    return -1;
  }
  return 0;
}

:class Server {

  struct {
    int sockfd;
  }
}

int
:create_socket(void) {
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket failed");
    exit(0);
  }
  return sockfd;
}

· *
:new(void) {
  · * self = calloc(1, sizeof(·));
  @class = &Server;
  @sockfd = Server.create_socket();
  return self;
}

void
:free(self) {
  close(@sockfd);
  free(self);
}

addr_in_t
:build_addr(self, uint16_t port) {
  addr_in_t addr = {0};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htons(INADDR_ANY);
  addr.sin_port = htons(port);
  return addr;
}

void
:bind(self, addr_in_t * addr) {
  int ret = bind(@sockfd, (addr_t *) addr,
                 sizeof(addr_t));
  if (ret == -1) {
    perror("bind failed");
    exit(0);
  }
}

void
:setsockopt(self) {
  int on = 1;
  int ret = setsockopt(@sockfd, SOL_SOCKET,
                       SO_REUSEADDR, &on, sizeof(on));
  if (ret == -1) {
    perror("setsockopt failed");
    exit(0);
  }
}

void
:listen(self, uint16_t port) {
  int ret = listen(@sockfd, 20);
  if (ret == -1) {
    perror("listen failed");
    exit(0);
  }
  printf("Listening on port %" PRIu16 "\n", port);
}

static jmp_buf catch = {0};

void
:interrupt(int signo) {
  longjmp(catch, 1);
}

void
:set_interrupt(void) {
  void (* sig)(int) = signal(SIGINT, Server.interrupt);
  if (sig == SIG_ERR) {
    perror("Can't catch SIGINT");
    exit(0);
  }
}

void
:accept(self) {
  Server.set_interrupt();
  int leave = setjmp(catch);
  if (leave) return;
  while (1) {
    addr_in_t client_addr = {0};
    int client_addr_len = {0};
    int fd = accept(@sockfd, (addr_t *) &client_addr, &client_addr_len);
    if (fd == -1) {
      perror("accept failed");
      continue;
    }
    int ret = Worker.thread(fd);
    if (ret == -1) continue;
  }
}

void
:run(uint16_t port) {
  · * self = Server.new();
  addr_in_t addr = ·build_addr(port);
  ·setsockopt;
  ·bind(&addr);
  ·listen(port);
  ·accept;
  ·free;
}

int
main(void) {
  Server.run(SERVER_PORT);
  return 0;
}
