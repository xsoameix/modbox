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

void
:close(self, char * err) {
  perror(err);
  longjmp(* @fail, 1);
}

void
:recv(self, void * buf, size_t len) {
  ssize_t ret = recv(@sockfd, buf, len, 0);
  if (ret == 0)        ·close("airweb disconnected");
  else if (ret == -1)  ·close("recv from airweb failed");
  else if (ret != len) ·close("recv from airweb failed");
}

void
:read(self, atcp_t * atcp) {
  ·recv(atcp, sizeof(* atcp));
  atcp->data = ntohs(atcp->data);
}

void
:print_mtcp(self, mbuf_t * buf, size_t len) {
  printf("recv");
  size_t i = 0;
  for (; i < len; i++) {
    printf(" %02X", ((uint8_t *) buf)[i]);
  }
  putchar('\n');
  fflush(stdout);
}

void
:send(self, void * buf, size_t len) {
  ssize_t ret = send(@sockfd, buf, len, 0);
  if (ret == -1)       ·close("send to airweb failed");
  else if (ret != len) ·close("send to airweb mismatch");
}

void
:write(self, mbuf_t * buf, size_t len) {
  ·send(OFFSET(buf, sizeof(mhead_t)), len - sizeof(mhead_t));
}

void
:resend(self, client_t * client, atcp_t * atcp) {
  mbuf_t buf = {0};
  size_t len = client·get_mtcp(&buf, atcp);
  ·print_mtcp(&buf, len);
  ·write(&buf, len);
}

int
:proxy(self, client_t * client, atcp_t * atcp) {
  ·read(atcp);
  if (atcp->op == ATCP_CLOSE) return 1;
  ·resend(client, atcp);
  return 0;
}

void *
:run(void * fd) {
  jmp_buf wfail = {0}, cfail = {0};
  · * self = Worker.new(fd, &wfail);
  client_t * client = 0;
  atcp_t atcp = {0};
  int wfailed = setjmp(wfail), cfailed = setjmp(cfail);
  if (cfailed) client·free;
  if (!wfailed) {
    client = Client.new(&cfail);
    client·connect;
    if (cfailed) ·resend(client, &atcp);
    while (·proxy(client, &atcp) == 0);
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
    perror("air server could not cerate thread");
    return -1;
  }
  return 0;
}

:class Server {

  struct {
    int sockfd;
  }
}

void
:close(self, char * err) {
  perror(err);
  ·free;
  exit(0);
}

int
:create_socket(self) {
  int sockfd = socket(PF_INET, SOCK_STREAM, 0);
  if (sockfd == -1) ·close("air server socket failed");
  return sockfd;
}

· *
:new(void) {
  · * self = calloc(1, sizeof(·));
  @class = &Server;
  @sockfd = ·create_socket;
  return self;
}

void
:free(self) {
  if (@sockfd) close(@sockfd);
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
  int ret = bind(@sockfd, (addr_t *) addr, sizeof(addr_t));
  if (ret == -1) ·close("air server bind failed");
}

void
:setsockopt(self) {
  int on = 1;
  int ret = setsockopt(@sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
  if (ret == -1) ·close("air server setsockopt failed");
}

void
:listen(self, uint16_t port) {
  int ret = listen(@sockfd, 20);
  if (ret == -1) ·close("listen failed");
  printf("Listening on port %" PRIu16 "\n", port);
}

static jmp_buf catch = {0};

void
:interrupt(int signo) {
  longjmp(catch, 1);
}

void
:set_interrupt(self) {
  void (* sig)(int) = signal(SIGINT, Server.interrupt);
  if (sig == SIG_ERR) ·close("air server can't catch SIGINT");
}

int
:check_ip(self, addr_in_t * client) {
  addr_in_t docker, mask;
  inet_aton("172.17.0.0", &docker.sin_addr);
  inet_aton("255.255.0.0.", &mask.sin_addr);
  return ((client->sin_addr.s_addr & mask.sin_addr.s_addr) !=
          (docker.sin_addr.s_addr & mask.sin_addr.s_addr));
}

void
:accept(self) {
  ·set_interrupt;
  int leave = setjmp(catch);
  if (leave) return;
  while (1) {
    addr_in_t client_addr = {0};
    socklen_t client_addr_len = sizeof(client_addr);
    int fd = accept(@sockfd, (addr_t *) &client_addr, &client_addr_len);
    if (fd == -1) {
      perror("air server accept failed");
      continue;
    } else if (·check_ip(&client_addr)) {
      perror("air server rejected");
      close(fd);
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
