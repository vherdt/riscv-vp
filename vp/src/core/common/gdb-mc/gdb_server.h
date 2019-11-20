#ifndef RISCV_GDB_NG
#define RISCV_GDB_NG

#include <stdint.h>
#include <stddef.h>

#include <queue>
#include <mutex>
#include <systemc>
#include <thread>
#include <map>
#include <tuple>

#include "debug.h"
#include "core_defs.h"
#include "gdb_stub.h" // DebugMemoryInterface
#include "protocol/protocol.h"

/* TODO: move this header to common? */
#include <platform/hifive/async_event.h>

SC_MODULE(GDBServer) {
public:
	typedef void (GDBServer::*packet_handler)(int, gdb_command_t *);

	void haltReason(int, gdb_command_t *);
	void getRegisters(int, gdb_command_t *);
	void setThread(int, gdb_command_t *);
	void readRegister(int, gdb_command_t *);
	void qAttached(int, gdb_command_t *);
	void qSupported(int, gdb_command_t *);
	void threadInfo(int, gdb_command_t *);
	void threadInfoEnd(int, gdb_command_t *);

	SC_HAS_PROCESS(GDBServer);

	GDBServer(sc_core::sc_module_name,
	          std::vector<debugable*>,
	          DebugMemoryInterface*,
	          uint16_t);

private:
	typedef std::function<void(debugable *)> thread_func;
	typedef std::tuple<int, gdb_packet_t *> ctx;

	AsyncEvent asyncEvent;
	Architecture arch;
	std::vector<debugable*> harts;
	std::thread thr;
	char *prevpkt;
	std::queue<ctx> pktq;
	std::mutex mtx;
	int sockfd;

	/* operation → thread id */
	std::map<char, int> thread_ops;

	void create_sock(uint16_t);
	void exec_thread(thread_func, char = 'g');
	void writeall(int, char *, size_t);
	void send_packet(int, const char *, gdb_kind_t = GDB_KIND_PACKET);
	void retransmit(int);
	void dispatch(int conn);
	void serve(void);
	void run(void);
};

#endif
