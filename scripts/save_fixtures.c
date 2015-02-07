#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <setjmp.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <check.h>
#include "../mtcp.h"

:require "../client.c"

:class SaveFixtures {}

typedef struct {
  char * name;
  uint8_t data;
} elem_t;

typedef struct {
  uint8_t begin;
  uint8_t end;
} range_t;

typedef struct {
  char * name;
  uint8_t data;
} data_t;

typedef struct {
  char * name;
  uint8_t addr;
} cmd_t;

typedef struct wcmd {
  cmd_t * cmd;
  uint8_t gaddr; // group address
  uint8_t (* gen_data)(struct wcmd *, uint8_t index, data_t * ret);
  void * data;
  uint8_t data_len;
} wcmd_t; // writable command

uint8_t
elem_data(wcmd_t * wcmd, uint8_t index, data_t * ret) {
  if (index >= wcmd->data_len) return 1;
  elem_t * elem = wcmd->data;
  ret->name = elem[index].name;
  ret->data = elem[index].data;
  return 0;
}

uint8_t
range_data(wcmd_t * wcmd, uint8_t index, data_t * ret) {
  range_t * range = wcmd->data;
  if (range->begin + index > range->end) return 1;
  ret->name = "range";
  ret->data = range->begin + index;
  return 0;
}

#define LENOF(ary) (sizeof(ary) / sizeof(ary[0]))
#define LASTOF(ary) ((ary)[LENOF(ary) - 1])
#define ARGSOF(ary) ary, sizeof(ary[0]), LENOF(ary)
#define ELEM(cmd, gaddr, data) {&cmd, gaddr, elem_data, data, LENOF(data)}
#define RANGE(cmd, gaddr, data) {&cmd, gaddr, range_data, &data, UINT8_MAX}

cmd_t power      = {"power",     0x10};
cmd_t lock       = {"lock",      0x11};
cmd_t mode       = {"mode",      0x12};
cmd_t fan_speed  = {"fan_speed", 0x13};
cmd_t alarm      = {"alarm",     0x14};
cmd_t valve      = {"valve",     0x15};
cmd_t fan_out    = {"fan_out",   0x16};
cmd_t chill      = {"chill",     0x17};
cmd_t heat       = {"heat",      0x18};
cmd_t uv         = {"uv",        0x19};
cmd_t room_temp  = {"room_temp", 0x1A};
cmd_t set_point  = {"set_point", 0x1B};
cmd_t * cmds[] = {
  &power, &lock, &mode, &fan_speed, &alarm, &valve,
  &fan_out, &chill, &heat, &uv, &room_temp, &set_point
};
elem_t  e_checkbox[]  = {{"off", 0}, {"on", 1}};
elem_t  e_mode[]      = {{"auto", 1}, {"cold", 2}, {"blash",  3}, {"warm", 4},
  {"wet", 5}};
elem_t  e_fan_speed[] = {{"auto", 1}, {"fast", 2}, {"mid",    3}, {"slow", 4}};
range_t e_point       = {20, 70};
wcmd_t wcmds[] = {
  ELEM(power,      0x7, e_checkbox),
  ELEM(lock,       0xB, e_checkbox),
  ELEM(mode,       0x9, e_mode),
  ELEM(fan_speed,  0xA, e_fan_speed),
  RANGE(set_point, 0x8, e_point)
};

#define GET 0x3
#define SET 0x6

void
save_head(FILE * fixture) {
  fprintf(fixture, "%s %s %-3s %-9s %-5s  %s\n",
          "unit", "panel", "op", "addr", "send", "recv");
}

void
recv_sock(FILE * fixture, client_t * client,
          mtcp_t * mtcp, atcp_t * atcp, char * addr, char * data) {
  fprintf(fixture, "%4u %5u %s %-9s %-5s ", atcp->unit, atcp->panel,
          atcp->op == GET ? "get" : "set", addr, data);
  uint8_t i = 0;
  for (; i < sizeof(mhead_t) + atcp->len; i++) {
    fprintf(fixture, " %02X", ((uint8_t *) mtcp)[i]);
  }
  fputc('\n', fixture);
  fflush(fixture);
}

void
send_sock(FILE * fixture, client_t * client,
          atcp_t * atcp, char * addr, char * data) {
  mtcp_t mtcp = client·get_mtcp(atcp);
  recv_sock(fixture, client, &mtcp, atcp, addr, data);
  printf(".");
  fflush(stdout);
}

void
save_err_op_snapshots(FILE * fixture, client_t * client) {
  atcp_t sock =     {.unit = 1,             .panel = 1,
    .op = INT8_MAX,  .addr = cmds[0]->addr, .data = 0x1};
  send_sock(fixture, client, &sock, "err op", "1");
}

void
save_read_err_snapshots(FILE * fixture, client_t * client) {
  atcp_t sock = {.unit = 1,         .panel = 1,
    .op = GET,   .addr = UINT8_MAX, .data = 0x1};
  send_sock(fixture, client, &sock, "err read", "1");
}

void
save_write_err_snapshots(FILE * fixture, client_t * client) {
  atcp_t sock = {.unit = 1,             .panel = 1,
    .op = SET,   .addr = cmds[0]->addr, .data = UINT16_MAX};
  send_sock(fixture, client, &sock, "err write", "1");
}

typedef struct {
  FILE * fixture;
  client_t * client;
  uint8_t unit;
  uint8_t panel;
} bind_t;

typedef void each_panel_t(void *, bind_t *);

void
for_each_cmds(void * cmds, size_t size, size_t len,
              each_panel_t * itor, bind_t * bind) {
  uint8_t unit = 1;
  for (; unit <= 2; unit++) {
    uint8_t panel = 1;
    for (; panel <= 2; panel++) {
      uint8_t cmd_i = 0;
      for (; cmd_i < len; cmd_i++) {
        bind->unit = unit;
        bind->panel = panel;
        itor(cmds + size * cmd_i, bind);
      }
    }
  }
}

void
itor_read_cmd(void * item, bind_t * bind) {
  cmd_t * cmd = * (cmd_t **) item;
  atcp_t sock = {.unit = bind->unit, .panel = bind->panel,
    .op = GET,   .addr = cmd->addr,  .data = 0x1};
  send_sock(bind->fixture, bind->client, &sock, cmd->name, "1");
}

void
save_read_snapshots(FILE * fixture, client_t * client) {
  bind_t bind = {fixture, client};
  for_each_cmds(ARGSOF(cmds), itor_read_cmd, &bind);
}

typedef struct wbind {
  bind_t bind;
  data_t * data;
  void (* itor)(wcmd_t *, struct wbind *);
} wbind_t; // write bind

void
itor_write_cmd(wcmd_t * wcmd, wbind_t * wbind) {
  bind_t bind = wbind->bind;
  data_t * data = wbind->data;
  cmd_t * cmd = wcmd->cmd;
  atcp_t sock = {.unit = bind.unit, .panel = bind.panel,
    .op = SET,   .addr = cmd->addr, .data = data->data};
  send_sock(bind.fixture, bind.client, &sock, cmd->name, data->name);
}

void
itor_data(void * item, bind_t * bind) {
  wcmd_t * wcmd = item;
  uint8_t i = 0;
  for (; ; i++) {
    data_t data = {0};
    uint8_t ret = wcmd->gen_data(wcmd, i, &data);
    if (ret) break;
    wbind_t * wbind = (wbind_t *) bind;
    wbind->data = &data;
    wbind->itor(wcmd, wbind);
  }
}

void
save_write_snapshots(FILE * fixture, client_t * client) {
  wbind_t bind = {{fixture, client}, .itor = itor_write_cmd};
  for_each_cmds(ARGSOF(wcmds), itor_data, (bind_t *) &bind);
}

void
itor_gwrite_cmd(wcmd_t * wcmd, wbind_t * wbind) {
  bind_t bind = wbind->bind;
  data_t * data = wbind->data;
  cmd_t * cmd = wcmd->cmd;
  atcp_t sock = {.unit = bind.unit,   .panel = bind.panel,
    .op = SET,   .addr = wcmd->gaddr, .data = data->data};
  send_sock(bind.fixture, bind.client, &sock, cmd->name, data->name);
}

void
for_each_wcmds(void * cmds, size_t size, size_t len,
               each_panel_t * itor, bind_t * bind) {
  uint8_t unit = 1;
  for (; unit <= 2; unit++) {
    uint8_t cmd_i = 0;
    for (; cmd_i < len; cmd_i++) {
      bind->unit = unit;
      bind->panel = 0;
      itor(cmds + size * cmd_i, bind);
    }
  }
}

void
save_gwrite_snapshots(FILE * fixture, client_t * client) {
  wbind_t bind = {{fixture, client}, .itor = itor_gwrite_cmd};
  for_each_wcmds(ARGSOF(wcmds), itor_data, (bind_t *) &bind);
}

void
save_fixtures(client_t * client) {
  FILE * fixture = fopen("fixtures", "w");
  if (fixture) {
    save_head(fixture);
    save_err_op_snapshots(fixture, client);
    save_read_err_snapshots(fixture, client);
    save_read_snapshots(fixture, client);
    save_write_err_snapshots(fixture, client);
    save_write_snapshots(fixture, client);
    save_gwrite_snapshots(fixture, client);
    putchar('\n');
    fclose(fixture);
  } else {
    perror("Cannot open `fixtures` file.");
  }
}

int
main(void) {
  jmp_buf fail = {0};
  client_t * client = 0;
  int failed = setjmp(fail);
  if (!failed) {
    client = Client.new(&fail);
    client·connect;
    save_fixtures(client);
  }
  client·free;
  return 0;
}
