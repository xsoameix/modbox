#include <setjmp.h>
#include "mtcp.h"
#include "config.h"

:require "server.c"

:class Main {}

int
main(void) {
  Server.run(SERVER_PORT);
  return 0;
}
