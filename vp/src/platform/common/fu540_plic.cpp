#include <assert.h>
#include <stdint.h>
#include <stddef.h>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"
#include "fu540_plic.h"

#define GET_IDX(IRQ) ((IRQ) / 32)
#define GET_OFF(IRQ) (1 << (IRQ) % 32)

enum {
	ENABLE_BASE = 0x2000,
	ENABLE_PER_HART = 0x80,

	CONTEXT_BASE = 0x200000,
	CONTEXT_PER_HART = 0x1000,

	HART_REG_SIZE = 2 * sizeof(uint32_t),
};

static void assert_addr(size_t start, size_t end, RegisterRange *range) {
	assert(range->start == start && range->end + 1 == end + 0x4);
}

FU540_PLIC::FU540_PLIC(sc_core::sc_module_name) {
	/* Values copied from FE310_PLIC */
	clock_cycle = sc_core::sc_time(10, sc_core::SC_NS);

	create_registers();
	tsock.register_b_transport(this, &FU540_PLIC::transport);

	SC_THREAD(run);
};

void FU540_PLIC::create_registers(void) {
	assert_addr(0x4, 0xD8, &regs_interrupt_priorities);
	assert_addr(0x1000, 0x1004, &regs_pending_interrupts);

	register_ranges.push_back(&regs_interrupt_priorities);
	register_ranges.push_back(&regs_pending_interrupts);

	/* create IRQ enable and context registers */
	create_hart_regs(ENABLE_BASE, ENABLE_PER_HART, enabled_irqs);
	create_hart_regs(CONTEXT_BASE, CONTEXT_PER_HART, irq_priority);
}

void FU540_PLIC::create_hart_regs(uint64_t addr, uint64_t inc, hartmap &map) {
	auto add_reg = [this] (uint64_t a) {
		RegisterRange *r = new RegisterRange(a, HART_REG_SIZE);
		register_ranges.push_back(r);
		return r;
	};

	for (size_t i = 0; i < FU540_PLIC_HARTS; i++) {
		RegisterRange *mreg, *sreg;

		mreg = add_reg(addr);
		sreg = mreg; /* for hart0 */

		if (i != 0) { /* hart 0 only supports m-mode interrupts */
			addr += inc;
			sreg = add_reg(addr);
		}

		map[i] = new HartConfig(*mreg, *sreg);
		addr += inc;
	}
}

void FU540_PLIC::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	delay += 4 * clock_cycle; /* copied from FE310_PLIC */
	vp::mm::route("FU540_PLIC", register_ranges, trans, delay);
};

void FU540_PLIC::gateway_trigger_interrupt(uint32_t irq) {
	if (irq > FU540_PLIC_NUMIRQ)
		throw std::invalid_argument("IRQ value is invalid");

	pending_interrupts[GET_IDX(irq)] |= GET_OFF(irq);
	e_run.notify(clock_cycle);
};

void FU540_PLIC::run(void) {
	for (;;) {
		sc_core::wait(e_run);

		for (size_t i = 0; i < FU540_PLIC_HARTS; i++) {
			// TODO
		}
	}
}

bool FU540_PLIC::HartConfig::is_enabled(unsigned int irq, PrivilegeLevel *level) {
	unsigned int idx = GET_IDX(irq);
	unsigned int off = GET_OFF(irq);

	PrivilegeLevel r = NULL;
	if (m_mode[idx] & off) {
		r = MachineMode;
	} else if (s_mode[idx] & off) {
		r = SupervisorMode;
	} else {
		return false;
	}

	if (level)
		*level = r;
	return true;
}
