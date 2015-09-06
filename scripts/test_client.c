#include <stdio.h>
#include <stdlib.h>
#include "../mtcp.h"

:require "../client.c"

:class TestClient {

  struct {
    client_t * client;
  }
}

void
:deliver(self, uint8_t * src, size_t slen) {
  uint8_t * dst;
  size_t dlen, i;
  @client·deliver(src, slen, (void **) &dst, &dlen);
  for (i = 0; i < slen; i++)
    printf("%02X%s", src[i], i == slen - 1 ? " => " : " ");
  for (i = 0; i < dlen; i++)
    printf("%02X%s", dst[i], i == dlen - 1 ? "\n" : " ");
  free(dst);
}

void
:sendto758(self) {
  setenv("CONNECT_ADDR", getenv("CONNECT_758_ADDR"), 1);
  @client·connect;
  // id: 1  func: 0x3  panel: 2  addr: power  len: 1
  ·deliver("\0\0\0\0\x00\x06\x01\x03\x01\x13\x00\x01", 12);
  @client·close;
}

void
:sendtopan(self) {
  setenv("CONNECT_ADDR", getenv("CONNECT_PAN_ADDR"), 1);
  @client·connect;
  // id: 1  func: 0x1  addr: power  len: 1
  ·deliver("\0\0\0\0\x00\x06\x01\x01\x00\x00\x00\x01", 12);
  @client·close;
}

int
main(void) {
  · * self = &TestClient();
  @client = Client.new();
  ·sendto758;
  ·sendtopan;
  @client·free;
  return 0;
}
