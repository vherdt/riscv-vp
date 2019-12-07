#ifndef LIBGDB_RESPONSE_H
#define LIBGDB_RESPONSE_H

#ifdef __cplusplus
extern "C" {
#endif

char *gdb_serialize(gdb_kind_t, const char *);

#ifdef __cplusplus
}
#endif

#endif
