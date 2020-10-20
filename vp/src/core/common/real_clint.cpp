#include "real_clint.h"

#define MSIP_BASE 0UL
#define MSIP_SIZE 4

#define MTIMECMP_BASE 0x400UL
#define MTIMECMP_SIZE 8

#define MTIME_BASE 0xBFF8UL
#define MTIME_SIZE 8

RealCLINT::RealCLINT(sc_core::sc_module_name, size_t numharts) {
	for (size_t i = 0; i < numharts; i++) {
		auto msip = new RegisterRange(MSIP_BASE + (i * MSIP_SIZE), MSIP_SIZE);
		auto mtimecmp = new RegisterRange(MTIMECMP_BASE + (i * MTIMECMP_SIZE), MTIMECMP_SIZE);

		register_ranges.push_back(msip);
		register_ranges.push_back(mtimecmp);

		hartconfs[i] = new HartConfig(*msip, *mtimecmp);
	}

	mtime = new RegisterRange(MTIME_BASE, MTIME_SIZE);
	register_ranges.push_back(mtime);
}

RealCLINT::~RealCLINT(void) {
	// TODO: free register ranges
	// TODO: free hartconfs
}

uint64_t RealCLINT::update_and_get_mtime(void) {
	return 0; /* TODO */
}

void RealCLINT::transport(tlm::tlm_generic_payload &trans, sc_core::sc_time &delay) {
	vp::mm::route("RealCLINT", register_ranges, trans, delay);
}
