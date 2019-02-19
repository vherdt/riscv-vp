#pragma once

#include <assert.h>
#include <stdint.h>

#include <unordered_map>
#include <stdexcept>

#include "util/common.h"
#include "trap.h"


struct csr_32 {
	uint32_t reg = 0;
};


struct csr_misa {

	csr_misa() {
		init();
	}

	union {
		uint32_t reg = 0;
		struct {
			unsigned extensions : 26;
			unsigned wiri : 4;
			unsigned mxl : 2;
		};
	};

	bool has_C_extension() {
		return extensions & (1 << 2);
	}

	void init() {
		extensions = 1 | (1 << 2) | (1 << 8) | (1 << 12);  // IMAC
		wiri = 0;
		mxl = 1;  // RV32
	}
};


struct csr_mvendorid {
	union {
		uint32_t reg = 0;
		struct {
			unsigned offset : 7;
			unsigned bank : 25;
		};
	};
};


struct csr_mstatus {
	union {
		uint32_t reg = 0;
		struct {
			unsigned uie : 1;
			unsigned sie : 1;
			unsigned wpri1 : 1;
			unsigned mie : 1;
			unsigned upie : 1;
			unsigned spie : 1;
			unsigned wpri2 : 1;
			unsigned mpie : 1;
			unsigned spp : 1;
			unsigned wpri3 : 2;
			unsigned mpp : 2;
			unsigned fs : 2;
			unsigned xs : 2;
			unsigned mprv : 1;
			unsigned sum : 1;
			unsigned mxr : 1;
			unsigned tvm : 1;
			unsigned tw : 1;
			unsigned txr : 1;
			unsigned wpri4 : 8;
			unsigned sd : 1;
		};
	};
};

struct csr_mtvec {
	union {
		uint32_t reg = 0;
		struct {
			unsigned mode : 2;   // WARL
			unsigned base : 30;  // WARL
		};
	};

	uint32_t get_base_address() {
		return base << 2;
	}

	enum Mode { Direct = 0, Vectored = 1 };

	void checked_write(uint32_t val) {
		reg = val;
		if (mode >= 1)
			mode = 0;
	}
};


struct csr_mie {
	union {
		uint32_t reg = 0;
		struct {
			unsigned usie : 1;
			unsigned ssie : 1;
			unsigned wpri1 : 1;
			unsigned msie : 1;

			unsigned utie : 1;
			unsigned stie : 1;
			unsigned wpri2 : 1;
			unsigned mtie : 1;

			unsigned ueie : 1;
			unsigned seie : 1;
			unsigned wpri3 : 1;
			unsigned meie : 1;

			unsigned wpri4 : 20;
		};
	};
};

struct csr_mip {
	inline bool any_pending() {
		return msip || mtip || meip;
	}

	union {
		uint32_t reg = 0;
		struct {
			unsigned usip : 1;
			unsigned ssip : 1;
			unsigned wiri1 : 1;
			unsigned msip : 1;

			unsigned utip : 1;
			unsigned stip : 1;
			unsigned wiri2 : 1;
			unsigned mtip : 1;

			unsigned ueip : 1;
			unsigned seip : 1;
			unsigned wiri3 : 1;
			unsigned meip : 1;

			unsigned wiri4 : 20;
		};
	};
};

struct csr_mepc {
	union {
		uint32_t reg = 0;
	};
};

struct csr_mcause {
	union {
		uint32_t reg = 0;
		struct {
			unsigned exception_code : 31;  // WLRL
			unsigned interrupt : 1;
		};
	};
};

struct csr_satp {
	union {
		uint32_t reg = 0;
		struct {
			unsigned mode : 1;  // WARL
			unsigned asid : 9;  // WARL
			unsigned ppn : 22;  // WARL
		};
	};
};

struct csr_pmpcfg {
	union {
		uint32_t reg = 0;
		struct {
			unsigned UNIMPLEMENTED : 24;  // WARL
			unsigned L0 : 1;              // WARL
			unsigned _wiri0 : 2;          // WIRI
			unsigned A0 : 2;              // WARL
			unsigned X0 : 1;              // WARL
			unsigned W0 : 1;              // WARL
			unsigned R0 : 1;              // WARL
		};
	};
};

/*
 * Add new subclasses with specific consistency check (e.g. by adding virtual
 * write_low, write_high functions) if necessary.
 */
struct csr_64 {
	union {
		uint64_t reg = 0;
		struct {
			int32_t low;
			int32_t high;
		};
	};

	void increment() {
		++reg;
	}
};


enum {
	// 64 bit readonly registers
	CSR_CYCLE_ADDR = 0xC00,
	CSR_CYCLEH_ADDR = 0xC80,
	CSR_TIME_ADDR = 0xC01,
	CSR_TIMEH_ADDR = 0xC81,
	CSR_INSTRET_ADDR = 0xC02,
	CSR_INSTRETH_ADDR = 0xC82,

	// shadows for the above CSRs
	CSR_MCYCLE_ADDR = 0xB00,
	CSR_MCYCLEH_ADDR = 0xB80,
	CSR_MTIME_ADDR = 0xB01,
	CSR_MTIMEH_ADDR = 0xB81,
	CSR_MINSTRET_ADDR = 0xB02,
	CSR_MINSTRETH_ADDR = 0xB82,

	// 32 bit machine CSRs
	CSR_MVENDORID_ADDR = 0xF11,
	CSR_MARCHID_ADDR = 0xF12,
	CSR_MIMPID_ADDR = 0xF13,
	CSR_MHARTID_ADDR = 0xF14,

	CSR_MSTATUS_ADDR = 0x300,
	CSR_MISA_ADDR = 0x301,
	CSR_MEDELEG_ADDR = 0x302,
    CSR_MIDELEG_ADDR = 0x303,
	CSR_MIE_ADDR = 0x304,
	CSR_MTVEC_ADDR = 0x305,
	CSR_MCOUNTEREN_ADDR = 0x306,

	CSR_MSCRATCH_ADDR = 0x340,
	CSR_MEPC_ADDR = 0x341,
	CSR_MCAUSE_ADDR = 0x342,
	CSR_MTVAL_ADDR = 0x343,
	CSR_MIP_ADDR = 0x344,

	CSR_PMPCFG0_ADDR = 0x3A0,
    CSR_PMPCFG1_ADDR = 0x3A1,
    CSR_PMPCFG2_ADDR = 0x3A2,
    CSR_PMPCFG3_ADDR = 0x3A3,

    CSR_PMPADDR0_ADDR = 0x3B0,
    CSR_PMPADDR1_ADDR = 0x3B1,
    CSR_PMPADDR2_ADDR = 0x3B2,
    CSR_PMPADDR3_ADDR = 0x3B3,
    CSR_PMPADDR4_ADDR = 0x3B4,
    CSR_PMPADDR5_ADDR = 0x3B5,
    CSR_PMPADDR6_ADDR = 0x3B6,
    CSR_PMPADDR7_ADDR = 0x3B7,
    CSR_PMPADDR8_ADDR = 0x3B8,
    CSR_PMPADDR9_ADDR = 0x3B9,
    CSR_PMPADDR10_ADDR = 0x3BA,
    CSR_PMPADDR11_ADDR = 0x3BB,
    CSR_PMPADDR12_ADDR = 0x3BC,
    CSR_PMPADDR13_ADDR = 0x3BD,
    CSR_PMPADDR14_ADDR = 0x3BE,
    CSR_PMPADDR15_ADDR = 0x3BF,

    // 32 bit supervisor CSRs
    CSR_SATP_ADDR = 0x180,
};


struct csr_table {
    csr_64 cycle;
    csr_64 time;
    csr_64 instret;

    csr_mvendorid mvendorid;
    csr_32 marchid;
    csr_32 mimpid;
    csr_32 mhartid;

	csr_mstatus mstatus;
	csr_misa misa;
	csr_mie mie;
	csr_mtvec mtvec;

	csr_32 mscratch;
	csr_mepc mepc;
	csr_mcause mcause;
	csr_32 mtval;
	csr_mip mip;

	// risc-v tests execution
	csr_32 mideleg;
	csr_32 medeleg;
	csr_32 pmpaddr0;
	csr_pmpcfg pmpcfg0;
	csr_satp satp;

	std::unordered_map<unsigned, uint32_t*> register_mapping;

	csr_table() {
        register_mapping[CSR_MVENDORID_ADDR] = &mvendorid.reg;
        register_mapping[CSR_MARCHID_ADDR] = &marchid.reg;
        register_mapping[CSR_MIMPID_ADDR] = &mimpid.reg;
        register_mapping[CSR_MHARTID_ADDR] = &mhartid.reg;
        register_mapping[CSR_MSTATUS_ADDR] = &mstatus.reg;
        register_mapping[CSR_MISA_ADDR] = &misa.reg;
        register_mapping[CSR_MIE_ADDR] = &mie.reg;
        register_mapping[CSR_MTVEC_ADDR] = &mtvec.reg;
        register_mapping[CSR_MSCRATCH_ADDR] = &mscratch.reg;
        register_mapping[CSR_MEPC_ADDR] = &mepc.reg;
        register_mapping[CSR_MCAUSE_ADDR] = &mcause.reg;
        register_mapping[CSR_MTVAL_ADDR] = &mtval.reg;
        register_mapping[CSR_MIP_ADDR] = &mip.reg;
        register_mapping[CSR_MIDELEG_ADDR] = &mideleg.reg;
        register_mapping[CSR_MEDELEG_ADDR] = &medeleg.reg;
        register_mapping[CSR_PMPADDR0_ADDR] = &pmpaddr0.reg;
        register_mapping[CSR_PMPCFG0_ADDR] = &pmpcfg0.reg;
        register_mapping[CSR_SATP_ADDR] = &satp.reg;
	}

	bool is_valid_csr32_addr(unsigned addr) {
	    return register_mapping.find(addr) != register_mapping.end();
	}

	void default_write32(unsigned addr, uint32_t value) {
	    auto it = register_mapping.find(addr);
	    ensure ((it != register_mapping.end()) && "validate address before calling this function");
	    *it->second = value;
	}

	uint32_t default_read32(unsigned addr) {
        auto it = register_mapping.find(addr);
        ensure ((it != register_mapping.end()) && "validate address before calling this function");
        return *it->second;
	}
};