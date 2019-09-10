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
	class FU540_HartConfig {
	  public:
		ArrayView<uint32_t> s_mode;
		ArrayView<uint32_t> m_mode;

		FU540_HartConfig(RegisterRange &r1, RegisterRange &r2) : s_mode(r1), m_mode(r2) {
			return;
		}
	};

	/* hart_id (0..4) → hart_config */
	typedef std::map<unsigned int, FU540_HartConfig*> hartmap;
	hartmap enabled_irqs;
	hartmap irq_priority;
	void create_hart_regs(uint64_t, uint64_t, hartmap&);

	void transport(tlm::tlm_generic_payload&, sc_core::sc_time&);

	std::vector<RegisterRange*> register_ranges;

	/* See Section 10.3 */
	RegisterRange regs_interrupt_priorities{0x4, sizeof(uint32_t) * (FU540_PLIC_NUMIRQ + 1)};
	ArrayView<uint32_t> interrupt_priorities{regs_interrupt_priorities};

	/* See Section 10.4 */
	RegisterRange regs_pending_interrupts{0x1000, sizeof(uint32_t) * 2};
	ArrayView<uint32_t> pending_interrupts{regs_pending_interrupts};
};

#endif
