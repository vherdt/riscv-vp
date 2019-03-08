#ifndef RISCV_ISA_GDB_STUB_H
#define RISCV_ISA_GDB_STUB_H

#include "iss.h"


struct DebugMemoryInterface : public sc_core::sc_module {
    tlm_utils::simple_initiator_socket<DebugMemoryInterface> isock;

    DebugMemoryInterface(sc_core::sc_module_name) {
    }

    bool _do_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes);

    std::string read_memory(uint64_t start, unsigned nbytes);

    void write_memory(uint64_t start, unsigned nbytes, const std::string &data);
};


//template <typename Core, unsigned Arch>
struct DebugCoreRunner : public sc_core::sc_module {
	ISS &core;
	RegFile &regfile;
	static constexpr size_t bufsize = 1024 * 8;
	char iobuf[bufsize]{};

	// access memory through a TLM socket
    DebugMemoryInterface *dbg_mem_if;

	SC_HAS_PROCESS(DebugCoreRunner);

	DebugCoreRunner(ISS &core, DebugMemoryInterface *mm)
	    : sc_module(sc_core::sc_module_name("DebugCoreRunner")), core(core), regfile(core.regs), dbg_mem_if(mm) {
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

#endif  // RISCV_ISA_GDB_STUB_H
