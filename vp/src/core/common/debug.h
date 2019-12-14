#ifndef RISCV_DEBUG
#define RISCV_DEBUG

#include <stdint.h>

#include <unordered_set>
#include <vector>

#include "core_defs.h"

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

	virtual void enable_debug(void) = 0;
	virtual CoreExecStatus get_status(void) = 0;
	virtual void set_status(CoreExecStatus) = 0;

	virtual void set_wfi(bool) = 0;
	virtual bool get_wfi(void) = 0;

	virtual void insert_breakpoint(uint64_t) = 0;
	virtual void remove_breakpoint(uint64_t) = 0;

	virtual Architecture get_architecture(void) = 0;
	virtual uint64_t get_hart_id(void) = 0;

	virtual uint64_t get_progam_counter(void) = 0;
	virtual std::vector<uint64_t> get_registers(void) = 0;
	virtual uint64_t read_register(unsigned) = 0;

	virtual void run(void) = 0;
};

#endif
