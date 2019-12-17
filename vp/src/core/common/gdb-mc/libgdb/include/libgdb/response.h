#ifndef LIBGDB_RESPONSE_H
#define LIBGDB_RESPONSE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Copied from QEMU
 * See: https://github.com/qemu/qemu/blob/d37147997/gdbstub.c#L103-L113 */
typedef enum {
	GDB_SIGNAL_0 = 0,
	GDB_SIGNAL_INT = 2,
	GDB_SIGNAL_QUIT = 3,
	GDB_SIGNAL_TRAP = 5,
	GDB_SIGNAL_ABRT = 6,
	GDB_SIGNAL_ALRM = 14,
	GDB_SIGNAL_IO = 23,
	GDB_SIGNAL_XCPU = 24,
	GDB_SIGNAL_UNKNOWN = 143
} gdb_signal_t;

char *gdb_serialize(gdb_kind_t, const char *);

#ifdef __cplusplus
}
#endif

#endif
