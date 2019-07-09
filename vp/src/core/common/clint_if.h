#pragma once

#include <stdint.h>

struct clint_if {
	virtual ~clint_if() {}

	virtual uint64_t update_and_get_mtime() = 0;
};