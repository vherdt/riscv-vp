#include <assert.h>
#include <stdint.h>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"
#include "fu540_plic.h"

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
	assert_addr(0x4, 0xD8, &regs_interrupt_priorities);
	assert_addr(0x1000, 0x1004, &regs_pending_interrupts);

	register_ranges.push_back(&regs_interrupt_priorities);
	register_ranges.push_back(&regs_pending_interrupts);

	/* create IRQ enable and context registers */
	create_hart_regs(ENABLE_BASE, ENABLE_PER_HART, enabled_irqs);
	create_hart_regs(CONTEXT_BASE, CONTEXT_PER_HART, irq_priority);

	tsock.register_b_transport(this, &FU540_PLIC::transport);
};

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
	vp::mm::route("FU540_PLIC", register_ranges, trans, delay);
};

void FU540_PLIC::gateway_trigger_interrupt(uint32_t irq) {
	if (irq > FU540_PLIC_NUMIRQ)
		throw std::invalid_argument("IRQ value is invalid");

	size_t idx = irq / 32;
	size_t off = irq % 32;

	pending_interrupts[idx] |= 1 << off;
};
