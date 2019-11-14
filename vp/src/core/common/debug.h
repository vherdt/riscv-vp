#ifndef RISCV_DEBUG
#define RISCV_DEBUG

#include <stdint.h>

/* TODO: For now the debugable class can only be used with RV64, it is
 * howver intended as an abstract interface that should work with both
 * RV32 and RV64.
 *
 * Unfourtunatly, supporting both would require significant changes to
 * iss.cpp, e.g. make sure they return a uniform type for register
 * values.
 */
struct debugable {
	virtual ~debugable() {}

	uint64_t get_hart_id(void);
};

#endif
