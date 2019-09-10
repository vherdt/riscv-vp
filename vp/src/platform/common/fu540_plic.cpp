#include <assert.h>
#include <stdint.h>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"
#include "fu540_plic.h"

static void assert_addr(size_t start, size_t end, RegisterRange *range) {
	std::cout << "start: " << start << " vs. " << range->start << std::endl;
	std::cout << "end: " << end +0x4 << " vs. " << range->end + 1 << std::endl;
	assert(range->start == start && range->end + 1 == end + 0x4);
}

FU540_PLIC::FU540_PLIC(sc_core::sc_module_name) {
	check_memory_layout();
	tsock.register_b_transport(this, &FU540_PLIC::transport);
};

void FU540_PLIC::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	(void)trans;
	(void)delay;
	return;
};

void FU540_PLIC::gateway_trigger_interrupt(uint32_t irq) {
	(void)irq;
	return;
};

void FU540_PLIC::check_memory_layout(void) {
	assert_addr(0x4, 0xD8, &regs_interrupt_priorities);
	assert_addr(0x1000, 0x1004, &regs_pending_interrupts);
	assert_addr(0x2000, 0x247c, &regs_hart_enabled_interrupts);
	assert_addr(0x200000, 0x208004, &regs_hart_config);
};
