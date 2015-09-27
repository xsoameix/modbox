#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdarg.h>
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

void
ninit(node_t * node) {
  node->prev = node->next = NULL;
}

void
linit(list_t * list) {
  list->head = list->tail = NULL;
}

void
lput(list_t * list, node_t * node) {
  if (list->head == NULL) {
    list->head = list->tail = node;
  } else {
    list->tail->next = node;
    node->prev = list->tail;
    list->tail = node;
  }
}

void
ldel(list_t * list, node_t * node) {
  if (list->head == NULL) return;
  if (node->prev == NULL)
    list->head = node->next;
  else
    node->prev->next = node->next;
  if (node->next == NULL)
    list->tail = node->prev;
  else
    node->next->prev = node->prev;
  ninit(node);
}

:require "client.c"

:class Worker {

  struct {
    node_t node; // MUST followed by `this` variable
    worker_t * this;
    node_t rnode; // ready node
    worker_t * rthis;
    pthread_cond_t cond;
    queue_t * queue;
    int sock;
    int intr;
    client_t * client;
    pthread_t pid;
    size_t id;
  }
}

void
lpri(list_t * list) {
  node_t * node;
  for (node = list->head; node != NULL; node = node->next)
    printf("%s%zu", node == list->head ? "" : " ",
           (* (worker_t **) (node + 1))->id);
}

· *
:new(int sock, client_t * client, queue_t * queue, int intr, size_t id) {
  · * self;
  if ((self = calloc(1, sizeof(·))) == NULL) {
    perror("worker calloc failed"); return NULL;
  }
  @class = &Worker;
  ninit(&@node);
  ninit(&@rnode);
  @this = self;
  @rthis = self;
  @intr = intr;
  @cond = (pthread_cond_t) PTHREAD_COND_INITIALIZER;
  @queue = queue;
  @sock = sock;
  @client = client;
  @id = id;
  return self;
}

void
:free(self) {
  puts("connection closed"), fflush(stdout);
  close(@sock);
  free(self);
}

void
:perror(self, char * msg) {
  char buf[80];
  snprintf(buf, sizeof(buf), "  worker %zu %s", @id, msg);
  perror(buf);
}

int
:recv(self, void * buf, size_t len) {
  size_t i;
  ssize_t ret;
  for (i = 0; i < len; i += ret) {
    ret = recv(@sock, buf + i, len - i, 0);
    if (ret == 0)  { ·perror("remote disconnected");     return 1; }
    if (ret == -1) { ·perror("recv from remote failed"); return 1; }
  }
  return 0;
}

int
:send(self, void * buf, size_t len) {
  size_t i;
  ssize_t ret;
  for (i = 0; i < len; i += ret)
    if ((ret = send(@sock, buf + i, len - i, 0)) == -1) {
      ·perror("send to remote failed"); return 1;
    }
  return 0;
}

int
:rselect(self, void * buf, size_t len) {
  int fdmax, i;
  fd_set readset;
  FD_ZERO(&readset);
  FD_SET(@intr, &readset);
  FD_SET(fdmax = @sock, &readset);
  if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
    perror("  worker select failed"); return 1;
  }
  for (i = 0; i < fdmax + 1; i++) {
    if (!FD_ISSET(i, &readset)) continue;
    if (i == @sock) return ·recv(buf, len);
    if (i == @intr) return 1;
  }
}

int
:sselect(self, void * buf, size_t len) {
  int fdmax, i;
  fd_set readset, writeset;
  FD_ZERO(&readset);
  FD_ZERO(&writeset);
  FD_SET(@intr, &readset);
  FD_SET(fdmax = @sock, &writeset);
  if (select(fdmax + 1, &readset, &writeset, NULL, NULL) == -1) {
    perror("  worker select failed"); return 1;
  }
  for (i = 0; i < fdmax + 1; i++) {
    if (!FD_ISSET(i, &readset) &&
        !FD_ISSET(i, &writeset)) continue;
    if (i == @sock) return ·send(buf, len);
    if (i == @intr) return 1;
  }
}

int
:recvmph(self, void ** data, size_t * len) {
  uint16_t pdulen;
  mph_t * head;
  if (·rselect(&pdulen, sizeof(uint16_t))) return 1;
  if (!(pdulen = ntohs(pdulen))) return 1;
  if ((head = * data = malloc(sizeof(mph_t) + pdulen)) == NULL) return 1;
  if (·rselect(* data + sizeof(mph_t), pdulen)) { free(* data); return 1; }
  head->txn = 0;
  head->proto = 0;
  head->len = htons(pdulen);
  * len = sizeof(mph_t) + pdulen;
  return 0;
}

int
:sendmph(self, void * data, size_t len) {
  return ·sselect(data, len);
}

void
:del(self) {
  pthread_mutex_lock(&@queue->run_mutex);
  ldel(&@queue->run, &@node);
  pthread_mutex_unlock(&@queue->run_mutex);
}

void *
:run(void * arg) {
  · * self = arg;
  void * src, * dst;
  size_t slen, dlen;
  int exit = 0, i = 0;
  while (1) {
    if (·recvmph(&src, &slen)) break;
    pthread_mutex_lock(&@queue->run_mutex);
    lput(&@queue->run, &@node);
    while (@queue->run.head != &@node)
      pthread_cond_wait(&@cond, &@queue->run_mutex);
    pthread_mutex_unlock(&@queue->run_mutex);
    printf("  worker %zu running", @id);
    printf(" queue: ["), lpri(&@queue->run);
    printf("]\n"), fflush(stdout);
    //sleep(1);
    //i++, exit = i == 4;
    if (@client·deliver(src, slen, &dst, &dlen)) { free(src), ·del; break; }
    ·del;
    if (@queue->run.head != NULL)
      pthread_cond_signal(&(* (worker_t **) (@queue->run.head + 1))->cond);
    if (·sendmph(dst, dlen)) { free(dst), free(src); break; }
    free(dst), free(src);
  }
  pthread_mutex_lock(&@queue->sleep_mutex);
  lput(&@queue->sleep, &@node);
  pthread_mutex_unlock(&@queue->sleep_mutex);
  pthread_cond_signal(&@queue->sleep_cond);
  return NULL;
}

:class Server {

  struct {
    int sock;
    int intr[2]; // used to close pthreads
    queue_t queue;
    client_t * client;
  }
}

· *
:new(void) {
  · * self;
  if ((self = calloc(1, sizeof(·))) == NULL) {
    perror("server calloc failed"); return NULL;
  }
  @class = &Server;
  if (pipe(@intr) == -1) {
    perror("server pipe failed"), free(self); return NULL;
  }
  linit(&@queue.run);
  linit(&@queue.ready);
  linit(&@queue.sleep);
  @queue.run_mutex   = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  @queue.sleep_mutex = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;
  @queue.sleep_cond  = (pthread_cond_t)  PTHREAD_COND_INITIALIZER;
  @queue.cleanup = 0;
  @queue.finished = 0;
  @client = NULL;
  return self;
}

void *
:clean(void * arg) {
  · * self = arg;
  node_t * node;
  worker_t * worker;
  int exit;
  for (exit = 0; !exit; exit = @queue.ready.head == NULL && @queue.finished) {
    pthread_mutex_lock(&@queue.sleep_mutex);
    while (@queue.sleep.head == NULL &&
           !(@queue.ready.head == NULL && @queue.finished)) {
      pthread_cond_wait(&@queue.sleep_cond, &@queue.sleep_mutex);
    }
    if (@queue.sleep.head != NULL) {
      worker = * (worker_t **) (@queue.sleep.head + 1);
      if (pthread_join(worker->pid, NULL))
        perror("server pthread_join worker failed");
      printf("  worker %zu deleted\n", worker->id), fflush(stdout);
      ldel(&@queue.sleep, &worker->node);
      ldel(&@queue.ready, &worker->rnode);
      worker·free;
    }
    if (@queue.run.head != NULL)
      pthread_cond_signal(&(* (worker_t **) (@queue.run.head + 1))->cond);
    pthread_mutex_unlock(&@queue.sleep_mutex);
  }
  return NULL;
}

void
:free(self) {
  if (@queue.cleanup) {
    write(@intr[1], "\0", 1);
    pthread_mutex_lock(&@queue.sleep_mutex);
    @queue.finished = 1;
    pthread_mutex_unlock(&@queue.sleep_mutex);
    pthread_cond_signal(&@queue.sleep_cond);
    if (pthread_join(@queue.cleaner, NULL))
      perror("server pthread_join cleaner failed");
    printf("worker all deleted\n"), fflush(stdout);
  }
  if (@client != NULL) @client·free;
  close(@sock);
  close(@intr[1]), close(@intr[0]);
  free(self);
  puts("Exit");
}

int
:listen(self) {
  addr_info_t hint = {0}, * res;
  int on = 1;
  char addr[INET6_ADDRSTRLEN];
  in_port_t port;
  hint.ai_family = AF_UNSPEC;
  hint.ai_socktype = SOCK_STREAM;
  getaddrinfo(Common.fetch_env_addr("SENDER_ADDR", "127.0.0.1"),
              Common.fetch_env_port("SENDER_PORT", "60000"), &hint, &res);
  if ((@sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol))==-1){
    perror("server socket failed");
    freeaddrinfo(res); return 1;
  }
  if (setsockopt(@sock, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) == -1) {
    perror("server setsockopt 'SO_REUSEADDR' failed");
    close(@sock), freeaddrinfo(res); return 1;
  }
  if (res->ai_addr->sa_family == AF_INET6 &&
      setsockopt(@sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)) == -1) {
    perror("server setsockopt 'IPV6_V6ONLY' failed");
    close(@sock), freeaddrinfo(res); return 1;
  }
  if (bind(@sock, res->ai_addr, res->ai_addrlen) == -1) {
    perror("server bind failed");
    close(@sock), freeaddrinfo(res); return 1;
  }
  if (listen(@sock, 20) == -1) {
    perror("listen failed");
    close(@sock), freeaddrinfo(res); return 1;
  }
  Common.addr_ntop(res->ai_addr, addr, sizeof(addr));
  Common.port_ntop(res->ai_addr, &port);
  printf("Listening on %s:%" PRIu16 "\n", addr, port);
  fflush(stdout);
  freeaddrinfo(res);
  return 0;
}

void
:getintr(int signo) {
  printf("receive %s signal\n", strsignal(signo)), fflush(stdout);
}

void
:ignpipe(int signo) {}

void
:setintr(void) {
  struct sigaction action;
  action.sa_handler = Server.getintr;
  action.sa_flags = 0;
  sigemptyset(&action.sa_mask);
  if (sigaction(SIGINT, &action, NULL) == -1)
    perror("server cannot catch SIGINT");
  if (sigaction(SIGTERM, &action, NULL) == -1)
    perror("server cannot catch SIGTERM");
  action.sa_handler = Server.ignpipe;
  if (sigaction(SIGPIPE, &action, NULL) == -1)
    perror("server cannot catch SIGPIPE");
}

void
:thread(self, int sock, size_t id) {
  worker_t * worker;
  if ((worker = Worker.new(sock, @client, &@queue, @intr[0], id)) == NULL) {
    close(sock); return;
  }
  if (pthread_create(&worker->pid, NULL, Worker.run, worker) == -1) {
    perror("server could not cerate thread");
    worker·free; return;
  }
  pthread_mutex_lock(&@queue.sleep_mutex);
  lput(&@queue.ready, &worker->rnode);
  printf("  worker %zu created\n", worker->id), fflush(stdout);
  pthread_mutex_unlock(&@queue.sleep_mutex);
}

void
:accept(self, size_t id) {
  addr_store_t addr = {0};
  socklen_t addrlen = sizeof(addr);
  int sock;
  if ((sock = accept(@sock, (addr_t *) &addr, &addrlen)) == -1) {
    perror("server accept failed"); return;
  }
  puts("connection created"), fflush(stdout);
  ·thread(sock, id);
}

void
:select(self) {
  int fdmax, i;
  fd_set readset;
  size_t id = 0;
  if ((@client = Client.new()) == NULL) return;
  if (pthread_create(&@queue.cleaner, NULL, Server.clean, self) == -1) {
    perror("server could not create thread"); return;
  }
  @queue.cleanup = 1;
  for (Server.setintr();;) {
    FD_ZERO(&readset);
    FD_SET(fdmax = @sock, &readset);
    if (select(fdmax + 1, &readset, NULL, NULL, NULL) == -1) {
      if (errno != EINTR) perror("server select failed");
      return;
    }
    for (i = 0; i < fdmax + 1; i++) {
      if (!FD_ISSET(i, &readset)) continue;
      if (i == @sock) ·accept(id++);
    }
  }
}

void
:run(void) {
  · * self;
  if ((self = Server.new()) == NULL) return;
  if (·listen) return;
  ·select;
  ·free;
}
