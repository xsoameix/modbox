set(TEST_PROJECT "test_${PROJECT}")
enable_testing()
find_package(Check REQUIRED)
add_custom_command(OUTPUT recv.c.c
  COMMAND nitrogen recv.c DEPENDS recv.c)
# Error:
#   /usr/bin/ld: /usr/lib/gcc/x86_64-linux-gnu/4.8/../../../x86_64-linux-gnu/libcheck_pic.a(check_pack.o): undefined reference to symbol '__pthread_unregister_cancel@@GLIBC_2.3.3'
#add_executable(${TEST_PROJECT} recv.c.c)
#target_link_libraries(${TEST_PROJECT} ${LIBS} ${CHECK_LIBRARIES})
add_test(${TEST_PROJECT} ${CMAKE_CURRENT_BANARY_DIR}/${PROJECT})
