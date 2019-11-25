#ifndef GDB_PARSER_H
#define GDB_PARSER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>

#define GDB_CSUM_LEN 2

typedef enum {
	GDB_KIND_NOTIFY,
	GDB_KIND_PACKET,
	GDB_KIND_NACK,
	GDB_KIND_ACK,
} gdb_kind_t;

typedef struct {
	gdb_kind_t kind;
	char *data;
	char csum[GDB_CSUM_LEN];
} gdb_packet_t;

void gdb_free_packet(gdb_packet_t *);
gdb_packet_t *gdb_parse_pkt(FILE *);

enum {
	GDB_THREAD_UNSET = -2,
	GDB_THREAD_ANY = -1,
	GDB_THREAD_ALL = 0,
};

typedef struct {
	int pid; /* requires multiprocess feature */
	int tid;
} gdb_thread_t;

/* TODO: change value types to unsigned */
typedef struct {
	int addr;
	int length;
} gdb_memory_t;

typedef struct {
	char op;
	gdb_thread_t id;
} gdb_cmd_h_t;

typedef struct _gdb_vcont_t gdb_vcont_t;

struct _gdb_vcont_t {
	char action;
	int sig; /* TODO: make this unsigned */
	gdb_thread_t *thread;

	gdb_vcont_t *next; /* NULL on end */
};

typedef enum {
	GDB_ARG_NONE,
	GDB_ARG_VCONT,
	GDB_ARG_H,
	GDB_ARG_INT,
	GDB_ARG_MEMORY,
} gdb_argument_t;

typedef struct {
	char *name;
	gdb_argument_t type;

	union {
		gdb_vcont_t *vval;
		gdb_cmd_h_t hcmd;
		int ival;
		gdb_memory_t mem;
	} v;
} gdb_command_t;

void gdb_free_cmd(gdb_command_t *);
gdb_command_t *gdb_parse_cmd(gdb_packet_t *);

bool gdb_is_valid(gdb_packet_t *);
char *gdb_unescape(char *);
char *gdb_decode_runlen(char *);
char *gdb_serialize(gdb_kind_t, const char *);

#ifdef __cplusplus
}
#endif

#endif
