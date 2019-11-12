#ifndef RISCV_GDB_NG
#define RISCV_GDB_NG

#include <stdint.h>
#include <stddef.h>

#include <systemc>
#include <thread>
#include <map>

#include "debug.h"
#include "gdb_stub.h" // DebugMemoryInterface
#include "protocol/protocol.h"

SC_MODULE(GDBServer) {
public:
	typedef void (GDBServer::*packet_handler)(int, gdb_command_t *);

	void setThread(int, gdb_command_t *);
	void qSupported(int, gdb_command_t *);
	void vMustReplyEmpty(int, gdb_command_t *);

	SC_HAS_PROCESS(GDBServer);

	GDBServer(sc_core::sc_module_name,
	          std::vector<debugable*>,
	          DebugMemoryInterface*,
	          uint16_t);

private:
	std::thread thr;
	char *prevpkt;
	int sockfd;

	void create_sock(uint16_t);
	void writeall(int, char *, size_t);
	void send_packet(int, const char *, gdb_kind_t = GDB_KIND_PACKET);
	void retransmit(int);
	void dispatch(int conn);
	void serve(void);
};

#endif
