cmake_minimum_required(VERSION 2.8)

set(PROJECT modbox)
set(PACKAGE "${PROJECT}.package")

macro(with_ext _NAME)
  set(PROJECT_SOURCES ${PROJECT_SOURCES} "${_NAME}.c.c")
  set(PROJECT_DEPENDS ${PROJECT_DEPENDS} "${_NAME}.c")
endmacro(with_ext)
with_ext("common")
with_ext("client")
with_ext("server")
find_package(Threads)
add_library(${PACKAGE} ${PROJECT_SOURCES})
set(LIBS ${LIBS} ${PACKAGE} ${CMAKE_THREAD_LIBS_INIT})
add_custom_command(OUTPUT modbox.c.c ${PROJECT_SOURCES}
  COMMAND nitrogen modbox.c DEPENDS modbox.c ${PROJECT_DEPENDS})
add_executable(${PROJECT} modbox.c.c)
target_link_libraries(${PROJECT} ${LIBS})

add_subdirectory(scripts)
