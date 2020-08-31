#ifndef RISCV_VP_LOAD_IF_H
#define RISCV_VP_LOAD_IF_H

#include <stdint.h>
#include <stddef.h>

class load_if {
  public:
	virtual void load_data(const char *src, uint64_t dst_addr, size_t n) = 0;
	virtual void load_zero(uint64_t dst_addr, size_t n) = 0;
};

#endif
