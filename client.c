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
    int sock;
    int connected;
  }
}

· *
:new(void) {
  · * self;
  if ((self = calloc(1, sizeof(·))) == NULL) {
    perror("client calloc failed"); return NULL;
  }
  @class = &Client;
  @connected = 0;
  return self;
}

void
:close(self) {
  puts("modbus disconnected"), fflush(stdout);
  close(@sock);
  @connected = 0;
}

void
:free(self) {
  if (@connected) ·close;
  free(self);
}

int
:connect(self) {
  addr_info_t hint = {0}, * res;
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  getaddrinfo(Common.fetch_env_addr("CONNECT_ADDR", "127.0.0.1"),
              Common.fetch_env_port("CONNECT_PORT", "60001"), &hint, &res);
  if ((@sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol))==-1){
    perror("cilent socket failed"), freeaddrinfo(res); return 1;
  }
  if (connect(@sock, res->ai_addr, res->ai_addrlen) == -1) {
    perror("client connect failed"), close(@sock), freeaddrinfo(res); return 1;
  }
  puts("modbus connected"), fflush(stdout);
  @connected = 1;
  freeaddrinfo(res);
  return 0;
}

void
:connect_modbus(self) {
  size_t i;
  if (!@connected)
    for (i = 1; ·connect; i < 1024 && (i *= 2)) sleep(i);
}

int
:send(self, void * buf, size_t len) {
  size_t i;
  ssize_t ret;
  for (i = 0; i < len; i += ret)
    if ((ret = send(@sock, buf + i, len - i, 0)) == -1) {
      perror("send to modbus failed"); return 1;
    }
  return 0;
}

int
:recv(self, void * buf, size_t len) {
  size_t i;
  ssize_t ret;
  for (i = 0; i < len; i += ret) {
    ret = recv(@sock, buf + i, len - i, 0);
    if (ret == 0)  { perror("modbus disconnected"), @connected = 0; return 1; }
    if (ret == -1) { perror("recv from modbus failed"); return 1; }
  }
  return 0;
}

void
:sendm(self, void * data, size_t len) {
  size_t i;
  for (i = 1; ·send(data, len); i < 1024 && (i *= 2))
    ·close, sleep(i), ·connect;
}

void
:recvm(self, void ** data, size_t * len) {
  mph_t head;
  size_t i;
  for (i = 1; ; ·close, sleep(i), ·connect, i < 1024 && (i *= 2)) {
    if (·recv(&head, sizeof(head))) continue;
    head.txn = ntohs(head.txn);
    head.proto = ntohs(head.proto);
    head.len = ntohs(head.len);
    if ((* data = malloc(sizeof(uint16_t) + head.len)) == NULL) continue;
    if (·recv(* data + sizeof(uint16_t), head.len)) { free(* data); continue; }
    * (uint16_t *) * data = htons(head.len);
    * len = sizeof(uint16_t) + head.len;
    break;
  }
}

void
:deliver(self, void * src, size_t slen, void ** dst, size_t * dlen) {
  ·connect_modbus, ·sendm(src, slen), ·recvm(dst, dlen);
}
