#include <setjmp.h>
#include "mtcp.h"
#include "config.h"

:require "server.c"

:class Main {}

int
main(void) {
  Server.run();
  return 0;
}
