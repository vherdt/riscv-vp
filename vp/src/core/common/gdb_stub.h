#ifndef RISCV_ISA_GDB_STUB_H
#define RISCV_ISA_GDB_STUB_H

#include <type_traits>
#include <string>

#include <tlm_utils/simple_initiator_socket.h>
#include <systemc>

#include "core_defs.h"
#include "trap.h"


struct DebugMemoryInterface : public sc_core::sc_module {
    tlm_utils::simple_initiator_socket<DebugMemoryInterface> isock;

    DebugMemoryInterface(sc_core::sc_module_name) {
    }

    unsigned _do_dbg_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes);

    std::string read_memory(uint64_t start, unsigned nbytes);

    void write_memory(uint64_t start, unsigned nbytes, const std::string &data);
};


template <typename Core, unsigned Arch>
struct DebugCoreRunner : public sc_core::sc_module {
    static_assert(Arch == RV32 || Arch == RV64, "architecture not supported");

    typedef typename std::conditional<Arch == RV32, uint32_t, uint64_t>::type reg_t;

	Core &core;
	static constexpr size_t bufsize = 1024 * 8;
	char iobuf[bufsize]{};

	// access memory through a TLM socket
    DebugMemoryInterface *dbg_mem_if;
    unsigned port_number;

	SC_HAS_PROCESS(DebugCoreRunner);

	DebugCoreRunner(Core &core, DebugMemoryInterface *mm, unsigned port_number)
	    : sc_module(sc_core::sc_module_name("DebugCoreRunner")), core(core), dbg_mem_if(mm), port_number(port_number) {
		SC_THREAD(run);
		core.debug_mode = true;
	}

	void run();

private:
    std::string read_memory(unsigned start, int nbytes);
    void write_memory(unsigned start, int nbytes, const std::string &data);

	void send_packet(int conn, const std::string &msg);
	std::string receive_packet(int conn);

	void handle_gdb_loop(int conn);
};

#include "gdb_stub.tpp"

#endif  // RISCV_ISA_GDB_STUB_H
