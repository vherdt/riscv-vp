#ifndef RISCV_VP_REAL_CLINT_H
#define RISCV_VP_REAL_CLINT_H

#include <stdint.h>

#include <systemc>
#include <map>
#include <tlm_utils/simple_target_socket.h>

#include "platform/common/async_event.h"
#include "util/memory_map.h"
#include "clint_if.h"
#include "irq_if.h"

class RealCLINT : public clint_if, public sc_core::sc_module {
public:
	RealCLINT(sc_core::sc_module_name, size_t numharts);
	~RealCLINT(void);

	uint64_t update_and_get_mtime(void) override;
	uint64_t ticks_to_usec(uint64_t ticks);

private:
	class HartConfig {
	public:
		IntegerView<uint32_t> msip;
		IntegerView<uint64_t> mtimecmp;

		HartConfig(RegisterRange &_msip, RegisterRange &_mtimecmp)
			: msip(_msip), mtimecmp(_mtimecmp) {};
	};

	/* Memory-mapped registers */
	RegisterRange *mtime;
	std::map<unsigned int, HartConfig*> hartconfs;

	AsyncEvent asyncEvent;
	std::vector<RegisterRange*> register_ranges;

	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay);
};

#endif
