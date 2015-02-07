#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <check.h>
#include "../mtcp.h"

:require "client.c"

:class Test {}

START_TEST(client_send) {
} END_TEST

Suite *
recv_suite(void) {
  Suite * suite = suite_create("client");
  TCase * tcase = tcase_create("Core");
  tcase_add_test(tcase, client_send);
  tcase_set_timeout(tcase, 100.0);
  suite_add_tcase(suite, tcase);
  return suite;
}

Suite *
all_suite(void) {
  return suite_create("all");
}

int
main(void) {
  SRunner * runner = srunner_create(all_suite());
  srunner_add_suite(runner, recv_suite());
  srunner_run_all(runner, CK_NORMAL);
  size_t failed = srunner_ntests_failed(runner);
  srunner_free(runner);
  return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
