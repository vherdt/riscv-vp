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

/* This is used to quantize a 1MHz value to the closest 32768Hz value */
#define DIVIDEND (uint64_t(15625)/uint64_t(512))

RealCLINT::RealCLINT(sc_core::sc_module_name, std::vector<clint_interrupt_target*> &_harts)
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

	last_mtime = std::chrono::high_resolution_clock::now();
	tsock.register_b_transport(this, &RealCLINT::transport);
}

RealCLINT::~RealCLINT(void) {
	return;
}

uint64_t RealCLINT::update_and_get_mtime(void) {
	time_point now = std::chrono::high_resolution_clock::now();
	usecs duration = std::chrono::duration_cast<usecs>(now - last_mtime);

	return usec_to_ticks(duration);
}

uint64_t RealCLINT::usec_to_ticks(usecs usec) {
	// This algorithm is inspired by the implementation provided by RIOT-OS.
	// https://github.com/RIOT-OS/RIOT/blob/d382bd656569599691c1a3e1c9b1662e07cf1a42/sys/include/xtimer/tick_conversion.h#L100-L106

	// XXX: Handle 64-Bit overflow?
	auto count = usec.count();
	uint64_t microseconds = (uint64_t)count;

	return microseconds / DIVIDEND;
}

std::chrono::microseconds RealCLINT::ticks_to_usec(uint64_t ticks) {
	// See comment in RealCLINT::usec_to_ticks
	return usecs(ticks * DIVIDEND);
}

void RealCLINT::post_write_mtimecmp(RegisterRange::WriteInfo info) {
	return; /* TODO */
}

void RealCLINT::post_write_msip(RegisterRange::WriteInfo info) {
	assert(info.addr % 4 == 0);
	unsigned hart = info.addr / 4;

	msip.at(hart) &= MSIP_MASK;
	harts.at(hart)->trigger_software_interrupt(msip.at(hart) != 0);
}

void RealCLINT::post_write_mtime(RegisterRange::WriteInfo info) {
	return; /* TODO */
}

void RealCLINT::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	vp::mm::route("RealCLINT", register_ranges, trans, delay);
}
