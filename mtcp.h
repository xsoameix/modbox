#ifndef MTCP_H
#define MTCP_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef uint8_t byte_t;
typedef struct sockaddr_in addr_in_t;
typedef struct sockaddr    addr_t;

#define air_tcp_struct \
  uint8_t unit; \
  union { \
    int8_t err; \
    uint8_t op; \
  }; \
  uint8_t panel; \
  uint8_t addr; \
  uint16_t data

typedef struct {
  uint8_t close;
  uint8_t len;
  air_tcp_struct;
} atcp_t; // air tcp

#define modbus_head_struct \
  uint16_t txn;  /* transaction id */ \
  uint16_t prot; /* protocol */ \
  uint16_t len

typedef struct {
  modbus_head_struct;
} mhead_t; // modbus header

typedef struct {
  modbus_head_struct;
  air_tcp_struct;
} mtcp_t; // modbus tcp

#define ATCP_GROUP 0x0
#define ATCP_SET 0x6

#endif
