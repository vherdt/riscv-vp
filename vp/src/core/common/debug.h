#ifndef RISCV_DEBUG
#define RISCV_DEBUG

#include <stdint.h>
#include <vector>

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

	virtual uint64_t get_hart_id(void) = 0;
	virtual uint64_t get_program_counter(void) = 0;

	virtual std::vector<int64_t> get_registers(void) = 0;

	virtual void run(void) = 0;
};

#endif
