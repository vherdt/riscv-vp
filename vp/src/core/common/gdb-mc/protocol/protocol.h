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
gdb_packet_t *gdb_parse(FILE *);

bool gdb_is_valid(gdb_packet_t *);
char *gdb_unescape(char *);
char *gdb_decode_runlen(char *);
char *gdb_serialize(gdb_kind_t, const char *);

#ifdef __cplusplus
}
#endif

#endif
