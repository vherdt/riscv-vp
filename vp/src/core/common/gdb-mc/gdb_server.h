#ifndef RISCV_GDB_NG
#define RISCV_GDB_NG

#include <stdio.h>
#include <stdint.h>

#include <systemc>
#include <thread>

#include "debug.h"
#include "gdb_stub.h" // DebugMemoryInterface

SC_MODULE(GDBServer) {
public:
	SC_HAS_PROCESS(GDBServer);

	GDBServer(sc_core::sc_module_name,
	          std::vector<debugable*>,
	          DebugMemoryInterface*,
	          uint16_t);

private:
	std::thread thr;
	int sockfd;

	void create_sock(uint16_t);
	void dispatch(FILE *);
	void serve(void);
};

#endif
