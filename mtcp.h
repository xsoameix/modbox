#ifndef MTCP_H
#define MTCP_H

#include <sys/socket.h>
#include <netinet/in.h>

typedef struct sockaddr_in addr_in_t;
typedef struct sockaddr    addr_t;

#define mhead_struct \
  uint8_t head[6]
#define ahead_struct \
  uint8_t unit; \
  union { \
    int8_t err; \
    uint8_t op; \
  }
#define abody_struct \
  uint8_t panel; \
  uint8_t addr; \
  uint16_t data

typedef struct {
  ahead_struct;
  abody_struct;
} atcp_t; // air tcp

typedef struct {
  mhead_struct;
  ahead_struct;
  uint8_t size;
} mget_t; // modbus get tcp

typedef struct {
  mhead_struct;
  ahead_struct;
  abody_struct;
} mset_t; // modbus set tcp

typedef struct {
  mhead_struct;
} mhead_t;

typedef struct {
  uint16_t txn;  /* transaction id */
  uint16_t prot; /* protocol */
  uint16_t len;
} mhead_in_t;

typedef mset_t mtcp_t;

// max bytes of data get from modbus
typedef uint8_t mbuf_t[sizeof(mget_t) + 0xF * sizeof(uint16_t)];

#define ATCP_GROUP     0x0 // panel number
#define ATCP_GET       0x3 // builtin address
#define ATCP_CLOSE    0xFF // custom address

#define OFFSET(struct_ptr, offset) ((uint8_t *) struct_ptr + offset)

#endif
