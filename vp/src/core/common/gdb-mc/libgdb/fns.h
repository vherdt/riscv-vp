#ifndef GDB_PROTOCOL_FNS
#define GDB_PROTOCOL_FNS

#include <stddef.h>

void *xrealloc(void *, size_t);
void *xmalloc(size_t);
char *xstrdup(char *);

bool gdb_is_valid(gdb_packet_t *);
char *gdb_unescape(char *);
char *gdb_decode_runlen(char *);

#endif
