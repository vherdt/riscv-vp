#include <assert.h>
#include <stdint.h>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"
#include "fu540_plic.h"

static void assert_addr(size_t start, size_t end, RegisterRange *range) {
	assert(range->start == start && range->end + 1 == end + 0x4);
}

FU540_PLIC::FU540_PLIC(sc_core::sc_module_name) {
	assert_addr(0x4, 0xD8, &regs_interrupt_priorities);
	assert_addr(0x1000, 0x1004, &regs_pending_interrupts);

	register_ranges.push_back(&regs_interrupt_priorities);
	register_ranges.push_back(&regs_pending_interrupts);

	create_hart_regs(0x2000, 0x80, enabled_irqs); /* Section 10.5 */
	create_hart_regs(0x200000, 0x1000, irq_priority); /* Section 10.6 and 10.7 */

	tsock.register_b_transport(this, &FU540_PLIC::transport);
};

void FU540_PLIC::create_hart_regs(uint64_t addr, uint64_t inc, std::map<unsigned int, FU540_HartConfig*> &map) {
	uint64_t size = 2 * sizeof(uint32_t);
	for (size_t i = 0; i < FU540_PLIC_HARTS; i++) {
		RegisterRange *mreg, *sreg;

		mreg = new RegisterRange(addr, size);
		register_ranges.push_back(mreg);

		if (i != 0) { /* hart 0 only supports m-mode interrupts */
			addr += inc;
			sreg = new RegisterRange(addr, size);
			register_ranges.push_back(sreg);
		} else {
			sreg = mreg;
		}

		map[i] = new FU540_HartConfig(*mreg, *sreg);
		addr += inc;
	}
}

void FU540_PLIC::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	vp::mm::route("FU540_PLIC", register_ranges, trans, delay);
};

void FU540_PLIC::gateway_trigger_interrupt(uint32_t irq) {
	(void)irq;
	return;
};
