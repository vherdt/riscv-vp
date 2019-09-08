#include <stdint.h>

#include <tlm_utils/simple_target_socket.h>
#include <systemc>

#include "core/common/irq_if.h"
#include "util/memory_map.h"
#include "util/tlm_map.h"
#include "fu540_plic.h"

FU540_PLIC::FU540_PLIC(sc_core::sc_module_name) {
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
