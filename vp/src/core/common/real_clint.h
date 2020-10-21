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
	RealCLINT(sc_core::sc_module_name, std::vector<clint_interrupt_target>&);
	~RealCLINT(void);

	tlm_utils::simple_target_socket<RealCLINT> tsock;
	uint64_t update_and_get_mtime(void) override;
private:
	RegisterRange regs_msip;
	RegisterRange regs_mtimecmp;
	RegisterRange regs_mtime;

	ArrayView<uint32_t> msip;
	ArrayView<uint64_t> mtimecmp;
	IntegerView<uint64_t> mtime;

	AsyncEvent asyncEvent;
	std::vector<RegisterRange*> register_ranges;
	std::vector<clint_interrupt_target> &harts;

	void post_write_mtimecmp(RegisterRange::WriteInfo info);
	void post_write_msip(RegisterRange::WriteInfo info);
	void post_write_mtime(RegisterRange::WriteInfo info);

	uint64_t ticks_to_usec(uint64_t ticks);
	void transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay);
};

#endif
