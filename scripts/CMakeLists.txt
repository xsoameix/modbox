add_custom_command(OUTPUT test_client.c.c
  COMMAND nitrogen test_client.c DEPENDS test_client.c)
add_executable(test_client test_client.c.c)
target_link_libraries(test_client ${LIBS})
