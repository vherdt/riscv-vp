#pragma once

#include <assert.h>
#include <stdint.h>

#include <unordered_map>
#include <stdexcept>

#include "util/common.h"
#include "trap.h"


enum class PrivilegeLevel {
	Machine,
	Supervisor,
	User
};


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

struct csr_mcounteren {
	union {
		uint32_t reg = 0;
		struct {
			unsigned CY : 1;
			unsigned TM : 1;
			unsigned IR : 1;
			unsigned reserved : 29;
		};
	};
};

struct csr_mcountinhibit {
	union {
		uint32_t reg = 0;
		struct {
			unsigned CY : 1;
			unsigned zero : 1;
			unsigned IR : 1;
			unsigned reserved : 29;
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
	CSR_MCOUNTINHIBIT_ADDR = 0x320,

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
    CSR_SSTATUS_ADDR = 0x100,
	CSR_SEDELEG_ADDR = 0x102,
	CSR_SIDELEG_ADDR = 0x103,
	CSR_SIE_ADDR = 0x104,
	CSR_STVEC_ADDR = 0x105,
	CSR_SCOUNTEREN_ADDR = 0x106,
	CSR_SSCRATCH_ADDR = 0x140,
	CSR_SEPC_ADDR = 0x141,
	CSR_SCAUSE_ADDR = 0x142,
	CSR_STVAL_ADDR = 0x143,
	CSR_SIP_ADDR = 0x144,
    CSR_SATP_ADDR = 0x180,

    // 32 bit user CSRs
    CSR_USTATUS_ADDR = 0x000,
	CSR_UIE_ADDR = 0x004,
	CSR_UTVEC_ADDR = 0x005,
	CSR_USCRATCH_ADDR = 0x040,
	CSR_UEPC_ADDR = 0x041,
	CSR_UCAUSE_ADDR = 0x042,
	CSR_UTVAL_ADDR = 0x043,
	CSR_UIP_ADDR = 0x044,


    // performance counters
    /*
for i in range(3,32):
	print("CSR_HPMCOUNTER{}_ADDR = 0x{:X},".format(i, 0xC00+i))

print("")
for i in range(3,32):
	print("CSR_HPMCOUNTER{}H_ADDR = 0x{:X},".format(i, 0xC80+i))

print("")
for i in range(3,32):
	print("CSR_MHPMCOUNTER{}_ADDR = 0x{:X},".format(i, 0xB00+i))

print("")
for i in range(3,32):
	print("CSR_MHPMCOUNTER{}H_ADDR = 0x{:X},".format(i, 0xB80+i))
     */
    CSR_HPMCOUNTER3_ADDR = 0xC03,
	CSR_HPMCOUNTER4_ADDR = 0xC04,
	CSR_HPMCOUNTER5_ADDR = 0xC05,
	CSR_HPMCOUNTER6_ADDR = 0xC06,
	CSR_HPMCOUNTER7_ADDR = 0xC07,
	CSR_HPMCOUNTER8_ADDR = 0xC08,
	CSR_HPMCOUNTER9_ADDR = 0xC09,
	CSR_HPMCOUNTER10_ADDR = 0xC0A,
	CSR_HPMCOUNTER11_ADDR = 0xC0B,
	CSR_HPMCOUNTER12_ADDR = 0xC0C,
	CSR_HPMCOUNTER13_ADDR = 0xC0D,
	CSR_HPMCOUNTER14_ADDR = 0xC0E,
	CSR_HPMCOUNTER15_ADDR = 0xC0F,
	CSR_HPMCOUNTER16_ADDR = 0xC10,
	CSR_HPMCOUNTER17_ADDR = 0xC11,
	CSR_HPMCOUNTER18_ADDR = 0xC12,
	CSR_HPMCOUNTER19_ADDR = 0xC13,
	CSR_HPMCOUNTER20_ADDR = 0xC14,
	CSR_HPMCOUNTER21_ADDR = 0xC15,
	CSR_HPMCOUNTER22_ADDR = 0xC16,
	CSR_HPMCOUNTER23_ADDR = 0xC17,
	CSR_HPMCOUNTER24_ADDR = 0xC18,
	CSR_HPMCOUNTER25_ADDR = 0xC19,
	CSR_HPMCOUNTER26_ADDR = 0xC1A,
	CSR_HPMCOUNTER27_ADDR = 0xC1B,
	CSR_HPMCOUNTER28_ADDR = 0xC1C,
	CSR_HPMCOUNTER29_ADDR = 0xC1D,
	CSR_HPMCOUNTER30_ADDR = 0xC1E,
	CSR_HPMCOUNTER31_ADDR = 0xC1F,

	CSR_HPMCOUNTER3H_ADDR = 0xC83,
	CSR_HPMCOUNTER4H_ADDR = 0xC84,
	CSR_HPMCOUNTER5H_ADDR = 0xC85,
	CSR_HPMCOUNTER6H_ADDR = 0xC86,
	CSR_HPMCOUNTER7H_ADDR = 0xC87,
	CSR_HPMCOUNTER8H_ADDR = 0xC88,
	CSR_HPMCOUNTER9H_ADDR = 0xC89,
	CSR_HPMCOUNTER10H_ADDR = 0xC8A,
	CSR_HPMCOUNTER11H_ADDR = 0xC8B,
	CSR_HPMCOUNTER12H_ADDR = 0xC8C,
	CSR_HPMCOUNTER13H_ADDR = 0xC8D,
	CSR_HPMCOUNTER14H_ADDR = 0xC8E,
	CSR_HPMCOUNTER15H_ADDR = 0xC8F,
	CSR_HPMCOUNTER16H_ADDR = 0xC90,
	CSR_HPMCOUNTER17H_ADDR = 0xC91,
	CSR_HPMCOUNTER18H_ADDR = 0xC92,
	CSR_HPMCOUNTER19H_ADDR = 0xC93,
	CSR_HPMCOUNTER20H_ADDR = 0xC94,
	CSR_HPMCOUNTER21H_ADDR = 0xC95,
	CSR_HPMCOUNTER22H_ADDR = 0xC96,
	CSR_HPMCOUNTER23H_ADDR = 0xC97,
	CSR_HPMCOUNTER24H_ADDR = 0xC98,
	CSR_HPMCOUNTER25H_ADDR = 0xC99,
	CSR_HPMCOUNTER26H_ADDR = 0xC9A,
	CSR_HPMCOUNTER27H_ADDR = 0xC9B,
	CSR_HPMCOUNTER28H_ADDR = 0xC9C,
	CSR_HPMCOUNTER29H_ADDR = 0xC9D,
	CSR_HPMCOUNTER30H_ADDR = 0xC9E,
	CSR_HPMCOUNTER31H_ADDR = 0xC9F,

	CSR_MHPMCOUNTER3_ADDR = 0xB03,
	CSR_MHPMCOUNTER4_ADDR = 0xB04,
	CSR_MHPMCOUNTER5_ADDR = 0xB05,
	CSR_MHPMCOUNTER6_ADDR = 0xB06,
	CSR_MHPMCOUNTER7_ADDR = 0xB07,
	CSR_MHPMCOUNTER8_ADDR = 0xB08,
	CSR_MHPMCOUNTER9_ADDR = 0xB09,
	CSR_MHPMCOUNTER10_ADDR = 0xB0A,
	CSR_MHPMCOUNTER11_ADDR = 0xB0B,
	CSR_MHPMCOUNTER12_ADDR = 0xB0C,
	CSR_MHPMCOUNTER13_ADDR = 0xB0D,
	CSR_MHPMCOUNTER14_ADDR = 0xB0E,
	CSR_MHPMCOUNTER15_ADDR = 0xB0F,
	CSR_MHPMCOUNTER16_ADDR = 0xB10,
	CSR_MHPMCOUNTER17_ADDR = 0xB11,
	CSR_MHPMCOUNTER18_ADDR = 0xB12,
	CSR_MHPMCOUNTER19_ADDR = 0xB13,
	CSR_MHPMCOUNTER20_ADDR = 0xB14,
	CSR_MHPMCOUNTER21_ADDR = 0xB15,
	CSR_MHPMCOUNTER22_ADDR = 0xB16,
	CSR_MHPMCOUNTER23_ADDR = 0xB17,
	CSR_MHPMCOUNTER24_ADDR = 0xB18,
	CSR_MHPMCOUNTER25_ADDR = 0xB19,
	CSR_MHPMCOUNTER26_ADDR = 0xB1A,
	CSR_MHPMCOUNTER27_ADDR = 0xB1B,
	CSR_MHPMCOUNTER28_ADDR = 0xB1C,
	CSR_MHPMCOUNTER29_ADDR = 0xB1D,
	CSR_MHPMCOUNTER30_ADDR = 0xB1E,
	CSR_MHPMCOUNTER31_ADDR = 0xB1F,

	CSR_MHPMCOUNTER3H_ADDR = 0xB83,
	CSR_MHPMCOUNTER4H_ADDR = 0xB84,
	CSR_MHPMCOUNTER5H_ADDR = 0xB85,
	CSR_MHPMCOUNTER6H_ADDR = 0xB86,
	CSR_MHPMCOUNTER7H_ADDR = 0xB87,
	CSR_MHPMCOUNTER8H_ADDR = 0xB88,
	CSR_MHPMCOUNTER9H_ADDR = 0xB89,
	CSR_MHPMCOUNTER10H_ADDR = 0xB8A,
	CSR_MHPMCOUNTER11H_ADDR = 0xB8B,
	CSR_MHPMCOUNTER12H_ADDR = 0xB8C,
	CSR_MHPMCOUNTER13H_ADDR = 0xB8D,
	CSR_MHPMCOUNTER14H_ADDR = 0xB8E,
	CSR_MHPMCOUNTER15H_ADDR = 0xB8F,
	CSR_MHPMCOUNTER16H_ADDR = 0xB90,
	CSR_MHPMCOUNTER17H_ADDR = 0xB91,
	CSR_MHPMCOUNTER18H_ADDR = 0xB92,
	CSR_MHPMCOUNTER19H_ADDR = 0xB93,
	CSR_MHPMCOUNTER20H_ADDR = 0xB94,
	CSR_MHPMCOUNTER21H_ADDR = 0xB95,
	CSR_MHPMCOUNTER22H_ADDR = 0xB96,
	CSR_MHPMCOUNTER23H_ADDR = 0xB97,
	CSR_MHPMCOUNTER24H_ADDR = 0xB98,
	CSR_MHPMCOUNTER25H_ADDR = 0xB99,
	CSR_MHPMCOUNTER26H_ADDR = 0xB9A,
	CSR_MHPMCOUNTER27H_ADDR = 0xB9B,
	CSR_MHPMCOUNTER28H_ADDR = 0xB9C,
	CSR_MHPMCOUNTER29H_ADDR = 0xB9D,
	CSR_MHPMCOUNTER30H_ADDR = 0xB9E,
	CSR_MHPMCOUNTER31H_ADDR = 0xB9F,

	CSR_MHPMEVENT3_ADDR = 0x323,
	CSR_MHPMEVENT4_ADDR = 0x324,
	CSR_MHPMEVENT5_ADDR = 0x325,
	CSR_MHPMEVENT6_ADDR = 0x326,
	CSR_MHPMEVENT7_ADDR = 0x327,
	CSR_MHPMEVENT8_ADDR = 0x328,
	CSR_MHPMEVENT9_ADDR = 0x329,
	CSR_MHPMEVENT10_ADDR = 0x32A,
	CSR_MHPMEVENT11_ADDR = 0x32B,
	CSR_MHPMEVENT12_ADDR = 0x32C,
	CSR_MHPMEVENT13_ADDR = 0x32D,
	CSR_MHPMEVENT14_ADDR = 0x32E,
	CSR_MHPMEVENT15_ADDR = 0x32F,
	CSR_MHPMEVENT16_ADDR = 0x330,
	CSR_MHPMEVENT17_ADDR = 0x331,
	CSR_MHPMEVENT18_ADDR = 0x332,
	CSR_MHPMEVENT19_ADDR = 0x333,
	CSR_MHPMEVENT20_ADDR = 0x334,
	CSR_MHPMEVENT21_ADDR = 0x335,
	CSR_MHPMEVENT22_ADDR = 0x336,
	CSR_MHPMEVENT23_ADDR = 0x337,
	CSR_MHPMEVENT24_ADDR = 0x338,
	CSR_MHPMEVENT25_ADDR = 0x339,
	CSR_MHPMEVENT26_ADDR = 0x33A,
	CSR_MHPMEVENT27_ADDR = 0x33B,
	CSR_MHPMEVENT28_ADDR = 0x33C,
	CSR_MHPMEVENT29_ADDR = 0x33D,
	CSR_MHPMEVENT30_ADDR = 0x33E,
	CSR_MHPMEVENT31_ADDR = 0x33F,
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
	csr_32 medeleg;
	csr_32 mideleg;
	csr_mie mie;
	csr_mtvec mtvec;
	csr_mcounteren mcounteren;
	csr_mcountinhibit mcountinhibit;

	csr_32 mscratch;
	csr_mepc mepc;
	csr_mcause mcause;
	csr_32 mtval;
	csr_mip mip;

	// pmp configuration
	csr_32 pmpaddr0;
	csr_pmpcfg pmpcfg0;

	// supervisor csrs (please note: some are already covered by the machine mode csrs, i.e. sstatus, sie and sip, and some are required but have the same fields, hence the machine mode classes are used)
	csr_mtvec stvec;
	csr_mcounteren scounteren;
	csr_32 sscratch;
	csr_mepc sepc;
	csr_mcause scause;
	csr_32 stval;
	csr_satp satp;

	// user csrs (see above comment)
	csr_mtvec utvec;
	csr_32 uscratch;
	csr_mepc uepc;
	csr_mcause ucause;
	csr_32 utval;


	std::unordered_map<unsigned, uint32_t*> register_mapping;

	csr_table() {
        register_mapping[CSR_MVENDORID_ADDR] = &mvendorid.reg;
        register_mapping[CSR_MARCHID_ADDR] = &marchid.reg;
        register_mapping[CSR_MIMPID_ADDR] = &mimpid.reg;
        register_mapping[CSR_MHARTID_ADDR] = &mhartid.reg;

        register_mapping[CSR_MSTATUS_ADDR] = &mstatus.reg;
        register_mapping[CSR_MISA_ADDR] = &misa.reg;
		register_mapping[CSR_MEDELEG_ADDR] = &medeleg.reg;
		register_mapping[CSR_MIDELEG_ADDR] = &mideleg.reg;
        register_mapping[CSR_MIE_ADDR] = &mie.reg;
        register_mapping[CSR_MTVEC_ADDR] = &mtvec.reg;
		register_mapping[CSR_MCOUNTEREN_ADDR] = &mcounteren.reg;
		register_mapping[CSR_MCOUNTINHIBIT_ADDR] = &mcountinhibit.reg;

        register_mapping[CSR_MSCRATCH_ADDR] = &mscratch.reg;
        register_mapping[CSR_MEPC_ADDR] = &mepc.reg;
        register_mapping[CSR_MCAUSE_ADDR] = &mcause.reg;
        register_mapping[CSR_MTVAL_ADDR] = &mtval.reg;
        register_mapping[CSR_MIP_ADDR] = &mip.reg;

        register_mapping[CSR_PMPADDR0_ADDR] = &pmpaddr0.reg;
        register_mapping[CSR_PMPCFG0_ADDR] = &pmpcfg0.reg;

		register_mapping[CSR_STVEC_ADDR] = &stvec.reg;
		register_mapping[CSR_SCOUNTEREN_ADDR] = &scounteren.reg;
		register_mapping[CSR_SSCRATCH_ADDR] = &sscratch.reg;
		register_mapping[CSR_SEPC_ADDR] = &sepc.reg;
		register_mapping[CSR_SCAUSE_ADDR] = &scause.reg;
		register_mapping[CSR_STVAL_ADDR] = &stval.reg;
        register_mapping[CSR_SATP_ADDR] = &satp.reg;

		register_mapping[CSR_UTVEC_ADDR] = &utvec.reg;
		register_mapping[CSR_USCRATCH_ADDR] = &uscratch.reg;
		register_mapping[CSR_UEPC_ADDR] = &uepc.reg;
		register_mapping[CSR_UCAUSE_ADDR] = &ucause.reg;
		register_mapping[CSR_UTVAL_ADDR] = &utval.reg;
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



#define SWITCH_CASE_MATCH_ANY_HPMCOUNTER_RV32	\
	case CSR_HPMCOUNTER3_ADDR:    \
	case CSR_HPMCOUNTER4_ADDR:    \
	case CSR_HPMCOUNTER5_ADDR:    \
	case CSR_HPMCOUNTER6_ADDR:    \
	case CSR_HPMCOUNTER7_ADDR:    \
	case CSR_HPMCOUNTER8_ADDR:    \
	case CSR_HPMCOUNTER9_ADDR:    \
	case CSR_HPMCOUNTER10_ADDR:   \
	case CSR_HPMCOUNTER11_ADDR:   \
	case CSR_HPMCOUNTER12_ADDR:   \
	case CSR_HPMCOUNTER13_ADDR:   \
	case CSR_HPMCOUNTER14_ADDR:   \
	case CSR_HPMCOUNTER15_ADDR:   \
	case CSR_HPMCOUNTER16_ADDR:   \
	case CSR_HPMCOUNTER17_ADDR:   \
	case CSR_HPMCOUNTER18_ADDR:   \
	case CSR_HPMCOUNTER19_ADDR:   \
	case CSR_HPMCOUNTER20_ADDR:   \
	case CSR_HPMCOUNTER21_ADDR:   \
	case CSR_HPMCOUNTER22_ADDR:   \
	case CSR_HPMCOUNTER23_ADDR:   \
	case CSR_HPMCOUNTER24_ADDR:   \
	case CSR_HPMCOUNTER25_ADDR:   \
	case CSR_HPMCOUNTER26_ADDR:   \
	case CSR_HPMCOUNTER27_ADDR:   \
	case CSR_HPMCOUNTER28_ADDR:   \
	case CSR_HPMCOUNTER29_ADDR:   \
	case CSR_HPMCOUNTER30_ADDR:   \
	case CSR_HPMCOUNTER31_ADDR:   \
	case CSR_HPMCOUNTER3H_ADDR:   \
	case CSR_HPMCOUNTER4H_ADDR:   \
	case CSR_HPMCOUNTER5H_ADDR:   \
	case CSR_HPMCOUNTER6H_ADDR:   \
	case CSR_HPMCOUNTER7H_ADDR:   \
	case CSR_HPMCOUNTER8H_ADDR:   \
	case CSR_HPMCOUNTER9H_ADDR:   \
	case CSR_HPMCOUNTER10H_ADDR:  \
	case CSR_HPMCOUNTER11H_ADDR:  \
	case CSR_HPMCOUNTER12H_ADDR:  \
	case CSR_HPMCOUNTER13H_ADDR:  \
	case CSR_HPMCOUNTER14H_ADDR:  \
	case CSR_HPMCOUNTER15H_ADDR:  \
	case CSR_HPMCOUNTER16H_ADDR:  \
	case CSR_HPMCOUNTER17H_ADDR:  \
	case CSR_HPMCOUNTER18H_ADDR:  \
	case CSR_HPMCOUNTER19H_ADDR:  \
	case CSR_HPMCOUNTER20H_ADDR:  \
	case CSR_HPMCOUNTER21H_ADDR:  \
	case CSR_HPMCOUNTER22H_ADDR:  \
	case CSR_HPMCOUNTER23H_ADDR:  \
	case CSR_HPMCOUNTER24H_ADDR:  \
	case CSR_HPMCOUNTER25H_ADDR:  \
	case CSR_HPMCOUNTER26H_ADDR:  \
	case CSR_HPMCOUNTER27H_ADDR:  \
	case CSR_HPMCOUNTER28H_ADDR:  \
	case CSR_HPMCOUNTER29H_ADDR:  \
	case CSR_HPMCOUNTER30H_ADDR:  \
	case CSR_HPMCOUNTER31H_ADDR:  \
	case CSR_MHPMCOUNTER3_ADDR:   \
	case CSR_MHPMCOUNTER4_ADDR:   \
	case CSR_MHPMCOUNTER5_ADDR:   \
	case CSR_MHPMCOUNTER6_ADDR:   \
	case CSR_MHPMCOUNTER7_ADDR:   \
	case CSR_MHPMCOUNTER8_ADDR:   \
	case CSR_MHPMCOUNTER9_ADDR:   \
	case CSR_MHPMCOUNTER10_ADDR:  \
	case CSR_MHPMCOUNTER11_ADDR:  \
	case CSR_MHPMCOUNTER12_ADDR:  \
	case CSR_MHPMCOUNTER13_ADDR:  \
	case CSR_MHPMCOUNTER14_ADDR:  \
	case CSR_MHPMCOUNTER15_ADDR:  \
	case CSR_MHPMCOUNTER16_ADDR:  \
	case CSR_MHPMCOUNTER17_ADDR:  \
	case CSR_MHPMCOUNTER18_ADDR:  \
	case CSR_MHPMCOUNTER19_ADDR:  \
	case CSR_MHPMCOUNTER20_ADDR:  \
	case CSR_MHPMCOUNTER21_ADDR:  \
	case CSR_MHPMCOUNTER22_ADDR:  \
	case CSR_MHPMCOUNTER23_ADDR:  \
	case CSR_MHPMCOUNTER24_ADDR:  \
	case CSR_MHPMCOUNTER25_ADDR:  \
	case CSR_MHPMCOUNTER26_ADDR:  \
	case CSR_MHPMCOUNTER27_ADDR:  \
	case CSR_MHPMCOUNTER28_ADDR:  \
	case CSR_MHPMCOUNTER29_ADDR:  \
	case CSR_MHPMCOUNTER30_ADDR:  \
	case CSR_MHPMCOUNTER31_ADDR:  \
	case CSR_MHPMCOUNTER3H_ADDR:  \
	case CSR_MHPMCOUNTER4H_ADDR:  \
	case CSR_MHPMCOUNTER5H_ADDR:  \
	case CSR_MHPMCOUNTER6H_ADDR:  \
	case CSR_MHPMCOUNTER7H_ADDR:  \
	case CSR_MHPMCOUNTER8H_ADDR:  \
	case CSR_MHPMCOUNTER9H_ADDR:  \
	case CSR_MHPMCOUNTER10H_ADDR: \
	case CSR_MHPMCOUNTER11H_ADDR: \
	case CSR_MHPMCOUNTER12H_ADDR: \
	case CSR_MHPMCOUNTER13H_ADDR: \
	case CSR_MHPMCOUNTER14H_ADDR: \
	case CSR_MHPMCOUNTER15H_ADDR: \
	case CSR_MHPMCOUNTER16H_ADDR: \
	case CSR_MHPMCOUNTER17H_ADDR: \
	case CSR_MHPMCOUNTER18H_ADDR: \
	case CSR_MHPMCOUNTER19H_ADDR: \
	case CSR_MHPMCOUNTER20H_ADDR: \
	case CSR_MHPMCOUNTER21H_ADDR: \
	case CSR_MHPMCOUNTER22H_ADDR: \
	case CSR_MHPMCOUNTER23H_ADDR: \
	case CSR_MHPMCOUNTER24H_ADDR: \
	case CSR_MHPMCOUNTER25H_ADDR: \
	case CSR_MHPMCOUNTER26H_ADDR: \
	case CSR_MHPMCOUNTER27H_ADDR: \
	case CSR_MHPMCOUNTER28H_ADDR: \
	case CSR_MHPMCOUNTER29H_ADDR: \
	case CSR_MHPMCOUNTER30H_ADDR: \
	case CSR_MHPMCOUNTER31H_ADDR: \
	case CSR_MHPMEVENT3_ADDR:     \
	case CSR_MHPMEVENT4_ADDR:     \
	case CSR_MHPMEVENT5_ADDR:     \
	case CSR_MHPMEVENT6_ADDR:     \
	case CSR_MHPMEVENT7_ADDR:     \
	case CSR_MHPMEVENT8_ADDR:     \
	case CSR_MHPMEVENT9_ADDR:     \
	case CSR_MHPMEVENT10_ADDR:    \
	case CSR_MHPMEVENT11_ADDR:    \
	case CSR_MHPMEVENT12_ADDR:    \
	case CSR_MHPMEVENT13_ADDR:    \
	case CSR_MHPMEVENT14_ADDR:    \
	case CSR_MHPMEVENT15_ADDR:    \
	case CSR_MHPMEVENT16_ADDR:    \
	case CSR_MHPMEVENT17_ADDR:    \
	case CSR_MHPMEVENT18_ADDR:    \
	case CSR_MHPMEVENT19_ADDR:    \
	case CSR_MHPMEVENT20_ADDR:    \
	case CSR_MHPMEVENT21_ADDR:    \
	case CSR_MHPMEVENT22_ADDR:    \
	case CSR_MHPMEVENT23_ADDR:    \
	case CSR_MHPMEVENT24_ADDR:    \
	case CSR_MHPMEVENT25_ADDR:    \
	case CSR_MHPMEVENT26_ADDR:    \
	case CSR_MHPMEVENT27_ADDR:    \
	case CSR_MHPMEVENT28_ADDR:    \
	case CSR_MHPMEVENT29_ADDR:    \
	case CSR_MHPMEVENT30_ADDR:    \
	case CSR_MHPMEVENT31_ADDR
