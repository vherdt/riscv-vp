#include "real_clint.h"

enum {
	MSIP_BASE = 0,
	MSIP_SIZE = 4,

	MTIMECMP_BASE = 0x400,
	MTIMECMP_SIZE = 8,

	MTIME_BASE = 0xBFF8,
	MTIME_SIZE = 8,
};

RealCLINT::RealCLINT(sc_core::sc_module_name, size_t numharts)
	: regs_msip(MSIP_BASE, MSIP_SIZE * numharts),
	  regs_mtimecmp(MTIMECMP_BASE, MTIMECMP_SIZE * numharts),
	  regs_mtime(MTIME_BASE, MTIME_SIZE),

	  msip(regs_msip),
	  mtimecmp(regs_mtimecmp),
	  mtime(regs_mtime) {
	regs_mtimecmp.alignment = 4;
	regs_msip.alignment = 4;
	regs_mtime.alignment = 4;

	tsock.register_b_transport(this, &RealCLINT::transport);
}

RealCLINT::~RealCLINT(void) {
	return;
}

uint64_t RealCLINT::update_and_get_mtime(void) {
	return 0; /* TODO */
}

void RealCLINT::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	vp::mm::route("RealCLINT", register_ranges, trans, delay);
}
