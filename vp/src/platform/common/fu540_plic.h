#ifndef RISCV_ISA_FU540_PLIC_H
#define RISCV_ISA_FU540_PLIC_H

#include <stdint.h>

enum {
	FU540_PLIC_HARTS  = 5,
	FU540_PLIC_NUMIRQ = 53,
};

struct FU540_PLIC : public sc_core::sc_module, public interrupt_gateway {
public:
	tlm_utils::simple_target_socket<FU540_PLIC> tsock;
	std::array<external_interrupt_target *, FU540_PLIC_HARTS> target_harts{};

	FU540_PLIC(sc_core::sc_module_name);
	void gateway_trigger_interrupt(uint32_t);

private:
	void check_memory_layout(void);
	void transport(tlm::tlm_generic_payload&, sc_core::sc_time&);

	struct __attribute__((__packed__)) HartConfig {
		uint32_t priority_threshold;
		uint32_t claim_response;
		uint32_t padding[2048];
	};

	/* See Section 10.3 */
	RegisterRange regs_interrupt_priorities{0x4, sizeof(uint32_t) * (FU540_PLIC_NUMIRQ + 1)};
	ArrayView<uint32_t> interrupt_priorities{regs_interrupt_priorities};

	/* See Section 10.4 */
	RegisterRange regs_pending_interrupts{0x1000, sizeof(uint32_t) * 2};
	ArrayView<uint32_t> pending_interrupts{regs_pending_interrupts};

	/* See Section 10.5 */
	RegisterRange regs_hart_enabled_interrupts{0x2000, ((2 * FU540_PLIC_HARTS) - 1) * 128};
	ArrayView<uint32_t> hart_enabled_interrupts{regs_hart_enabled_interrupts};

	/* See Section 10.6 and Section 10.7 */
	RegisterRange regs_hart_config{0x200000, sizeof(HartConfig) * FU540_PLIC_HARTS};
	ArrayView<HartConfig> hart_config{regs_hart_config};
};

#endif
