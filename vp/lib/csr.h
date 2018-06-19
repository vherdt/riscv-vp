#ifndef RISCV_ISA_CSR_INFO_H
#define RISCV_ISA_CSR_INFO_H

#include <stdint.h>
#include <assert.h>

#include <map>

#include <stdexcept>


inline void ensure(bool cond) {
    if (!cond)
        throw std::runtime_error("runtime assertion failed");
}


struct csr_base {

    enum class PrivilegeLevel {
        User = 0,
        Supervisor = 1,
        Reserved = 2,
        Machine = 3,
    };

    enum AccessMode {
        User = 1,
        Supervisor = 2,
        Machine = 4,
        Read  = 8,
        Write = 16
    };

    enum Internal {
        RW_BITS = 0xc00,   			// addr[11:10]
        MODE_BITS = 0x300, 			// addr[9:8]

        READONLY_BITS = 0x03,		// 0b11
        USER_BITS = 0x00,			// 0b00
        SUPERVISOR_BITS = 0x01,		// 0b01
        MACHINE_BITS = 0x03,		// 0b11
    };

    virtual ~csr_base() {}

    virtual int32_t unchecked_read() = 0;
    virtual void unchecked_write(int32_t val) = 0;

    virtual void init() {}

    int32_t read(PrivilegeLevel access_level=PrivilegeLevel::Machine) {
        ensure (level <= access_level);
        unchecked_read();
    }

    void write(int32_t val, PrivilegeLevel access_level=PrivilegeLevel::Machine) {
        ensure (level <= access_level);
        ensure (mode & Write);
        unchecked_write(val);
    }

    void clear_bits(int32_t mask, PrivilegeLevel access_level=PrivilegeLevel::Machine) {
        write(unchecked_read() & ~mask, access_level);
    }

    void set_bits(int32_t mask, PrivilegeLevel access_level=PrivilegeLevel::Machine) {
        write(unchecked_read() | mask, access_level);
    }

    csr_base(uint32_t addr, const char *name)
            : addr(addr), name(name) {
        mode  = _get_access_mode();
        level = (mode & User) ? PrivilegeLevel::User :
                (mode & Supervisor) ? PrivilegeLevel::Supervisor :
                (mode & Machine) ? PrivilegeLevel::Machine : PrivilegeLevel::Reserved;
        ensure (level != PrivilegeLevel::Reserved && "invalid privilege level");

        init();
    }

    AccessMode _get_access_mode() {
        int ans = Read;

        switch ((addr >> 8) & 0x03) {
            case USER_BITS:
                ans |= User;
                break;
            case SUPERVISOR_BITS:
                ans |= Supervisor;
                break;
            case MACHINE_BITS:
                ans |= Machine;
                break;
            default:
                ensure (false && "unknown access mode");
        }

        if ((addr & RW_BITS) != RW_BITS)
            ans |= Write;

        return static_cast<AccessMode>(ans);
    }

    uint32_t addr;
    const char *name;
    AccessMode mode;
    PrivilegeLevel level;
};


#define INCLUDE_CSR_MIXIN										\
	using csr_base::csr_base;									\
																\
	virtual int32_t unchecked_read() override {					\
		return reg;												\
	}															\
																\
	virtual void unchecked_write(int32_t val) override {		\
		reg = val;												\
	}



struct csr_32 : public csr_base {
    INCLUDE_CSR_MIXIN;

    int32_t reg = 0;
};


struct csr_misa : public csr_base {
    INCLUDE_CSR_MIXIN;

    union {
        int32_t reg = 0;
        struct {
            unsigned extensions: 26;
            unsigned wiri: 4;
            unsigned mxl: 2;
        };
    };

    virtual void init() override {
        extensions = 1 | (1 << 8) | (1 << 12);
        wiri = 0;
        mxl = 1;
    }
};


struct csr_mvendorid : public csr_base {
    INCLUDE_CSR_MIXIN;

    union {
        int32_t reg = 0;
        struct {
            unsigned offset: 7;
            unsigned bank: 25;
        };
    };
};



struct csr_mstatus : public csr_base {
    INCLUDE_CSR_MIXIN;

    union {
        int32_t reg = 0;
        struct {
            unsigned uie: 1;
            unsigned sie: 1;
            unsigned wpri1: 1;
            unsigned mie: 1;
            unsigned upie: 1;
            unsigned spie: 1;
            unsigned wpri2: 1;
            unsigned mpie: 1;
            unsigned spp: 1;
            unsigned wpri3: 2;
            unsigned mpp: 2;
            unsigned fs: 2;
            unsigned xs: 2;
            unsigned mprv: 1;
            unsigned sum: 1;
            unsigned mxr: 1;
            unsigned tvm: 1;
            unsigned tw: 1;
            unsigned txr: 1;
            unsigned wpri4: 8;
            unsigned sd: 1;
        };
    };
};



struct csr_mtvec : public csr_base {
    using csr_base::csr_base;

    union {
        int32_t reg = 0;
        struct {
            unsigned mode: 2;	// WARL
            unsigned base: 30;	// WARL
        };
    };

    uint32_t get_base_address() {
        return base << 2;
    }

    enum Mode {
        Direct = 0,
        Vectored = 1
    };

    virtual int32_t unchecked_read() override {
        return reg;
    }

    virtual void unchecked_write(int32_t val) override {
        reg = val;
        if (mode >= 1)
            mode = 0;
    }
};



struct csr_mie : public csr_base {
    INCLUDE_CSR_MIXIN;

    union {
        int32_t reg = 0;
        struct {
            unsigned usie: 1;
            unsigned ssie: 1;
            unsigned wpri1: 1;
            unsigned msie: 1;

            unsigned utie: 1;
            unsigned stie: 1;
            unsigned wpri2: 1;
            unsigned mtie: 1;

            unsigned ueie: 1;
            unsigned seie: 1;
            unsigned wpri3: 1;
            unsigned meie: 1;

            unsigned wpri4: 20;
        };
    };
};


struct csr_mip : public csr_base {
    INCLUDE_CSR_MIXIN;

    inline bool any_pending() {
        return msip || mtip || meip;
    }

    union {
        int32_t reg = 0;
        struct {
            unsigned usip: 1;
            unsigned ssip: 1;
            unsigned wiri1: 1;
            unsigned msip: 1;

            unsigned utip: 1;
            unsigned stip: 1;
            unsigned wiri2: 1;
            unsigned mtip: 1;

            unsigned ueip: 1;
            unsigned seip: 1;
            unsigned wiri3: 1;
            unsigned meip: 1;

            unsigned wiri4: 20;
        };
    };
};


struct csr_mepc : public csr_base {
    INCLUDE_CSR_MIXIN;

    union {
        int32_t reg = 0;
    };
};


struct csr_mcause : public csr_base {
    INCLUDE_CSR_MIXIN;

    union {
        int32_t reg = 0;
        struct {
            unsigned exception_code: 31;	// WLRL
            unsigned interrupt: 1;
        };
    };
};



/*
 * Add new subclasses with specific consistency check (e.g. by adding virtual write_low, write_high functions) if necessary.
 */
struct csr_64 {
    union {
        int64_t reg = 0;
        struct {
            int32_t low;
            int32_t high;
        };
    };

    void increment() {
        ++reg;
    }
};



struct csr_64_low : public csr_base {
    csr_64 &target;

    csr_64_low(csr_64 &obj, uint32_t addr, const char *name)
            : target(obj), csr_base(addr, name) {
    }

    virtual int32_t unchecked_read() override {
        return target.low;
    }

    virtual void unchecked_write(int32_t val) override {
        target.low = val;
    }
};


struct csr_64_high : public csr_base {
    csr_64 &target;

    csr_64_high(csr_64 &obj, uint32_t addr, const char *name)
        : target(obj), csr_base(addr, name) {
    }

    virtual int32_t unchecked_read() override {
        return target.high;
    }

    virtual void unchecked_write(int32_t val) override {
        target.high = val;
    }
};



enum csr_addresses {
    // 64 bit readonly registers
    CSR_CYCLE_ADDR = 0xC00,
    CSR_CYCLEH_ADDR = 0xC80,
    CSR_TIME_ADDR  = 0xC01,
    CSR_TIMEH_ADDR  = 0xC81,
    CSR_INSTRET_ADDR = 0xC02,
    CSR_INSTRETH_ADDR = 0xC82,

    // shadows for the above CSRs
    CSR_MCYCLE_ADDR = 0xB00,
    CSR_MCYCLEH_ADDR = 0xB80,
    CSR_MTIME_ADDR  = 0xB01,
    CSR_MTIMEH_ADDR  = 0xB81,
    CSR_MINSTRET_ADDR = 0xB02,
    CSR_MINSTRETH_ADDR = 0xB82,

    // 32 bit machine level ISA registers
    CSR_MVENDORID_ADDR = 0xF11,
    CSR_MARCHID_ADDR = 0xF12,
    CSR_MIMPID_ADDR = 0xF13,
    CSR_MHARTID_ADDR = 0xF14,

    CSR_MSTATUS_ADDR = 0x300,
    CSR_MISA_ADDR = 0x301,
    CSR_MIE_ADDR = 0x304,
    CSR_MTVEC_ADDR = 0x305,
    CSR_MSCRATCH_ADDR = 0x340,
    CSR_MEPC_ADDR = 0x341,
    CSR_MCAUSE_ADDR = 0x342,
    CSR_MTVAL_ADDR = 0x343,
    CSR_MIP_ADDR = 0x344,
};


struct csr_table {

#define CSR_TABLE_DEFINE_COUNTER(basename)      \
    csr_64 *basename ## _root = 0;              \
    csr_64_low *basename = 0;                   \
    csr_64_high *basename ## h = 0;

    /* user csrs */
    CSR_TABLE_DEFINE_COUNTER(cycle);
    CSR_TABLE_DEFINE_COUNTER(time);
    CSR_TABLE_DEFINE_COUNTER(instret);
    //CSR_TABLE_DEFINE_COUNTER(mcycle);
    //CSR_TABLE_DEFINE_COUNTER(minstret);

    /* machine csrs */
    csr_mvendorid *mvendorid = 0;
    csr_32 *marchid = 0;
    csr_32 *mimpid = 0;
    csr_32 *mhartid = 0;

    csr_mstatus *mstatus = 0;
    csr_misa *misa = 0;
    csr_mie *mie = 0;
    csr_mtvec *mtvec = 0;

    csr_32 *mscratch = 0;
    csr_mepc *mepc = 0;
    csr_mcause *mcause = 0;
    csr_32 *mtval = 0;
    csr_mip *mip = 0;

    std::map<uint32_t, csr_base*> addr_to_csr;

    csr_base &at(uint32_t addr) {
        auto *ans = addr_to_csr.at(addr);
        assert (ans != nullptr);
        return *ans;
    }

    template <typename T>
    T *_register(T *p) {
        addr_to_csr[p->addr] = p;
        return p;
    }

#define CSR_TABLE_ADD_COUNTER_64(basename, addr)                                            \
    basename ## _root = new csr_64();                                                       \
    basename = _register(new csr_64_low(*basename ## _root, addr, #basename));              \
    basename ## h = _register(new csr_64_high(*basename ## _root, addr+0x80, #basename));


    void setup() {
        /* user csrs */
        CSR_TABLE_ADD_COUNTER_64(cycle, 0xC00);
        CSR_TABLE_ADD_COUNTER_64(time, 0xC01);
        CSR_TABLE_ADD_COUNTER_64(instret, 0xC02);
        //CSR_TABLE_ADD_COUNTER_64(mcycle, 0xB00);
        //CSR_TABLE_ADD_COUNTER_64(minstret, 0xB02);

        /* machine csrs */
        mvendorid = _register(new csr_mvendorid(0xF11, "mvendorid"));
        marchid = _register(new csr_32(0xF12, "marchid"));
        mimpid = _register(new csr_32(0xF13, "mimpid"));
        mhartid = _register(new csr_32(0xF14, "mhartid"));

        mstatus = _register(new csr_mstatus(0x300, "mstatus"));
        misa = _register(new csr_misa(0x301, "misa"));

        mie = _register(new csr_mie(0x304, "mie"));
        mtvec = _register(new csr_mtvec(0x305, "mtvec"));

        mscratch = _register(new csr_32(0x340, "mscratch"));
        mepc = _register(new csr_mepc(0x341, "mepc"));
        mcause = _register(new csr_mcause(0x342, "mcause"));
        mtval = _register(new csr_32(0x343, "mtval"));
        mip = _register(new csr_mip(0x344, "mip"));
    }
};




#endif //RISCV_ISA_CSR_INFO_H
