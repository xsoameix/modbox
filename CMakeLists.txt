cmake_minimum_required(VERSION 2.8)

set(PROJECT air)
set(PACKAGE "${PROJECT}.package")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

macro(with_ext _NAME)
  set(AIR_SOURCES ${AIR_SOURCES} "${_NAME}.c.c")
  set(AIR_DEPENDS ${AIR_DEPENDS} "${_NAME}.c")
endmacro(with_ext)
with_ext("client")
with_ext("server")
find_package(Threads)
add_library(${PACKAGE} ${AIR_SOURCES})
set(LIBS ${LIBS} ${PACKAGE} ${CMAKE_THREAD_LIBS_INIT})
add_custom_command(OUTPUT air.c.c ${AIR_SOURCES}
  COMMAND nitrogen air.c DEPENDS air.c ${AIR_DEPENDS})
add_executable(${PROJECT} air.c.c)
target_link_libraries(${PROJECT} ${LIBS})

add_subdirectory(scripts)
add_subdirectory(test)
