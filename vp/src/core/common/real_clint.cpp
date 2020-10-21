#include <assert.h>
#include <stddef.h>

#include "real_clint.h"

enum {
	MSIP_BASE = 0,
	MSIP_SIZE = 4,

	MTIMECMP_BASE = 0x400,
	MTIMECMP_SIZE = 8,

	MTIME_BASE = 0xBFF8,
	MTIME_SIZE = 8,
};

enum {
	MSIP_MASK = 0x1, // The upper MSIP bits are tied to zero
};

RealCLINT::RealCLINT(sc_core::sc_module_name, std::vector<clint_interrupt_target> &_harts)
	: regs_msip(MSIP_BASE, MSIP_SIZE * _harts.size()),
	  regs_mtimecmp(MTIMECMP_BASE, MTIMECMP_SIZE * _harts.size()),
	  regs_mtime(MTIME_BASE, MTIME_SIZE),

	  msip(regs_msip),
	  mtimecmp(regs_mtimecmp),
	  mtime(regs_mtime),

	  harts(_harts) {
	regs_mtimecmp.alignment = 4;
	regs_msip.alignment = 4;
	regs_mtime.alignment = 4;

	register_ranges.push_back(&regs_mtimecmp);
	register_ranges.push_back(&regs_msip);
	register_ranges.push_back(&regs_mtime);

	regs_mtimecmp.post_write_callback = std::bind(&RealCLINT::post_write_mtimecmp, this, std::placeholders::_1);
	regs_msip.post_write_callback = std::bind(&RealCLINT::post_write_msip, this, std::placeholders::_1);

	tsock.register_b_transport(this, &RealCLINT::transport);
}

RealCLINT::~RealCLINT(void) {
	return;
}

uint64_t RealCLINT::update_and_get_mtime(void) {
	return 0; /* TODO */
}

uint64_t RealCLINT::ticks_to_usec(uint64_t ticks) {
	return ticks; /* TODO */
}

void RealCLINT::post_write_mtimecmp(RegisterRange::WriteInfo info) {
	return; /* TODO */
}

void RealCLINT::post_write_msip(RegisterRange::WriteInfo info) {
	assert(info.addr % 4 == 0);
	unsigned hart = info.addr / 4;

	msip.at(hart) &= MSIP_MASK;
	harts.at(hart).trigger_software_interrupt(msip.at(hart) != 0);
}

void RealCLINT::post_write_mtime(RegisterRange::WriteInfo info) {
	return; /* TODO */
}

void RealCLINT::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	vp::mm::route("RealCLINT", register_ranges, trans, delay);
}
