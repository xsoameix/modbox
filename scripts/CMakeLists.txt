add_custom_command(OUTPUT save_fixtures.c.c
  COMMAND nitrogen save_fixtures.c DEPENDS save_fixtures.c)
add_executable(save_fixtures save_fixtures.c.c)
target_link_libraries(save_fixtures ${LIBS})
