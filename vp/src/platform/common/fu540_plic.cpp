#include <assert.h>
#include <stdint.h>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"
#include "fu540_plic.h"

enum {
	FU540_IRQ_EN = 0x2000,
	FU540_PRIO   = 0x200000,
};

static void assert_addr(size_t start, size_t end, RegisterRange *range) {
	assert(range->start == start && range->end + 1 == end + 0x4);
}

FU540_PLIC::FU540_PLIC(sc_core::sc_module_name) {
	assert_addr(0x4, 0xD8, &regs_interrupt_priorities);
	assert_addr(0x1000, 0x1004, &regs_pending_interrupts);

	register_ranges.push_back(&regs_interrupt_priorities);
	register_ranges.push_back(&regs_pending_interrupts);

	create_enabled_regs();
	create_priority_regs();

	tsock.register_b_transport(this, &FU540_PLIC::transport);
};

void FU540_PLIC::create_enabled_regs(void) {
	uint64_t addr = FU540_IRQ_EN;
	uint64_t size = 2 * sizeof(uint32_t);

	for (size_t i = 0; i < FU540_PLIC_HARTS; i++) {
		RegisterRange *mreg, *sreg;

		mreg = new RegisterRange(addr, size);
		register_ranges.push_back(mreg);

		if (i != 0) { /* hart 0 only supports m-mode interrupts */
			addr += 0x80;
			sreg = new RegisterRange(addr, size);
			register_ranges.push_back(sreg);
		} else {
			sreg = mreg;
		}

		enabled_irqs[i] = new FU540_HartConfig(*mreg, *sreg);
		addr += 0x80;
	}
}

void FU540_PLIC::create_priority_regs(void) {
	uint64_t addr = FU540_PRIO;
	uint64_t size = 2 * sizeof(uint32_t);

	for (size_t i = 0; i < FU540_PLIC_HARTS; i++) {
		RegisterRange *mreg, *sreg;

		mreg = new RegisterRange(addr, size);
		register_ranges.push_back(mreg);

		if (i != 0) { /* hart 0 only supports m-mode interrupts */
			addr += 0x1000;
			sreg = new RegisterRange(addr, size);
			register_ranges.push_back(sreg);
		} else {
			sreg = mreg;
		}

		irq_priority[i] = new FU540_HartConfig(*mreg, *sreg);
		addr += 0x1000;
	}
}

void FU540_PLIC::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	vp::mm::route("FU540_PLIC", register_ranges, trans, delay);
};

void FU540_PLIC::gateway_trigger_interrupt(uint32_t irq) {
	(void)irq;
	return;
};
