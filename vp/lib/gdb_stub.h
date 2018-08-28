#ifndef RISCV_ISA_GDB_STUB_H
#define RISCV_ISA_GDB_STUB_H

#include "iss.h"


struct debug_memory_mapping {
    uint8_t *mem;
    uint32_t mem_offset;
    uint32_t mem_size;

    std::string read_memory(unsigned start, int nbytes);
    std::string zero_memory(int nbytes);

    void write_memory(unsigned start, int nbytes, const std::string &data);
};


struct DebugCoreRunner : public sc_core::sc_module {

    ISS &core;
    RegFile &regfile;
    static constexpr size_t bufsize = 1024 * 8;
    char* iobuf;

    // memory related information, allows direct memory access for debug mode
    debug_memory_mapping memory;

    SC_HAS_PROCESS(DebugCoreRunner);

    DebugCoreRunner(ISS &core, const debug_memory_mapping &mm)
            : sc_module(sc_core::sc_module_name("DebugCoreRunner")), core(core), regfile(core.regs), memory(mm) {
        SC_THREAD(run);
        core.debug_mode = true;
        iobuf = new char[bufsize];
    }

    ~DebugCoreRunner()
    {
    	delete[] iobuf;
    }

    void run();
    void send_packet(int conn, const std::string &msg);
    std::string receive_packet(int conn);

private:
    void handle_gdb_loop(int conn);
};


#endif //RISCV_ISA_GDB_STUB_H
