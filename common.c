#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <netdb.h>
#include "mtcp.h"

:class Common {}

void *
:get_addr(addr_t * addr) {
  if (addr->sa_family == AF_INET)
    return &((addr_in_t *) addr)->sin_addr;
  else
    return &((addr_in6_t *) addr)->sin6_addr;
}

void
:addr_ntop(addr_t * addr, char * str, size_t len) {
  inet_ntop(addr->sa_family, Common.get_addr(addr), str, len);
}

int
:valid_addr(const char * value) {
  addr_info_t hint = {0}, * res;
  int ret, valid = 1;
  hint.ai_family = AF_UNSPEC;
  hint.ai_flags = AI_NUMERICHOST;
  ret = getaddrinfo(value, NULL, &hint, &res);
  if (ret) return valid;
  if (res->ai_family == AF_INET || res->ai_family == AF_INET6) valid = 0;
  freeaddrinfo(res);
  return valid;
}

const char *
:fetch_env_addr(const char * key, const char * default_addr) {
  char * value = getenv(key);
  if (value == NULL || Common.valid_addr(value)) return default_addr;
  return value;
}

in_port_t
:get_port(addr_t * addr) {
  if (addr->sa_family == AF_INET) {
    return ((addr_in_t *) addr)->sin_port;
  } else {
    return ((addr_in6_t *) addr)->sin6_port;
  }
}

void
:port_ntop(addr_t * addr, in_port_t * port) {
  * port = ntohs(Common.get_port(addr));
}

int
:valid_port(const char * str) {
  in_port_t port = 0;
  size_t i, len = strlen(str);
  if (len > 5) return 1;
  for (i = 0; i < len; i++) if (!isdigit(str[i])) return 1;
  return 0;
}

const char *
:fetch_env_port(const char * key, const char * default_port) {
  char * value = getenv(key);
  if (value == NULL || Common.valid_port(value)) return default_port;
  return value;
}
