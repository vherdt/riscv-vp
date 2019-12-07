#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include <libgdb/parser2.h>

#include "debug.h"
#include "gdb_server.h"
#include "register_format.h"

enum {
	GDB_PKTSIZ = 4096,
	GDB_PC_REG = 32,
};

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "?", &GDBServer::haltReason },
	{ "g", &GDBServer::getRegisters },
	{ "H", &GDBServer::setThread },
	{ "k", &GDBServer::killServer },
	{ "m", &GDBServer::readMemory },
	{ "p", &GDBServer::readRegister },
	{ "qAttached", &GDBServer::qAttached },
	{ "qSupported", &GDBServer::qSupported },
	{ "qfThreadInfo", &GDBServer::threadInfo },
	{ "qsThreadInfo", &GDBServer::threadInfoEnd },
	{ "T", &GDBServer::isAlive },
	{ "vCont", &GDBServer::vCont },
	{ "vCont?", &GDBServer::vContSupported },
	{ "z", &GDBServer::removeBreakpoint },
	{ "Z", &GDBServer::setBreakpoint },
};

void GDBServer::haltReason(int conn, gdb_command_t *cmd) {
	// If n is a recognized stop reason […]. The aa should be
	// ‘05’, the trap signal. At most one stop reason should be
	// present.

	// TODO: Only send create conditionally.
	send_packet(conn, "S05");
}

void GDBServer::getRegisters(int conn, gdb_command_t *cmd) {
	auto formatter = new RegisterFormater(arch);
	auto fn = [formatter] (debugable *hart) {
		for (uint64_t v : hart->get_registers())
			formatter->formatRegister(v);
	};

	exec_thread(fn);
	send_packet(conn, formatter->str().c_str());
	delete formatter;
}

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	gdb_cmd_h_t *hcmd;

	hcmd = &cmd->v.hcmd;
	thread_ops[hcmd->op] = hcmd->id.tid;

	send_packet(conn, "OK");
}

void GDBServer::killServer(int conn, gdb_command_t *cmd) {
	// TODO: Stop the System C simulation instead of
	// terminating the program. This would require interacting
	// with the GDBServerRunner directly to make it exit.
	exit(EXIT_SUCCESS);
}

void GDBServer::readMemory(int conn, gdb_command_t *cmd) {
	gdb_memory_t *mem;

	mem = &cmd->v.mem;

	assert(mem->addr <= UINT64_MAX);
	assert(mem->length <= UINT_MAX);

	std::string val;
	try {
		val = memory->read_memory((uint64_t)mem->addr, (unsigned)mem->length);
	} catch (const std::runtime_error&) {
		send_packet(conn, "E01");
		return;
	}

	send_packet(conn, val.c_str());
}

void GDBServer::readRegister(int conn, gdb_command_t *cmd) {
	int reg;
	debugable *hart;

	reg = cmd->v.ival;
	auto fn = [this, reg, conn] (debugable *hart) {
		uint64_t regval;
		RegisterFormater formatter(arch);

		if (reg == GDB_PC_REG) {
			regval = hart->pc;
		} else {
			regval = hart->read_register(reg);
		}

		/* TODO: handle CSRs? */

		formatter.formatRegister(regval);
		send_packet(conn, formatter.str().c_str());
	};

	try {
		exec_thread(fn);
	} catch (const std::out_of_range&) {
		send_packet(conn, "E01");
	}
}

void GDBServer::threadInfo(int conn, gdb_command_t *cmd) {
	std::string thrlist = "m";

	/* TODO: refactor this to make it always output hex digits,
	 * preferablly move it to the protocol code/ */
	for (size_t i = 0; i < harts.size(); i++) {
		thrlist += std::to_string(i + 1);
		if (i + 1 < harts.size())
			thrlist += ",";
	}

	send_packet(conn, thrlist.c_str());
}

void GDBServer::threadInfoEnd(int conn, gdb_command_t *cmd) {
	// GDB will respond to each reply with a request for more thread
	// ids (using the ‘qs’ form of the query), until the target
	// responds with ‘l’ (lower-case ell, for last).
	//
	// Since the GDBServer::threadInfo sends all threads in one
	// response we always terminate the list here.
	send_packet(conn, "l");
}

void GDBServer::qAttached(int conn, gdb_command_t *cmd) {
	// 0 process started, 1 attached to process
	send_packet(conn, "0");
}

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, ("vContSupported+;PacketSize=" + std::to_string(GDB_PKTSIZ)).c_str());
}

void GDBServer::isAlive(int conn, gdb_command_t *cmd) {
	gdb_thread_t *thr;

	thr = &cmd->v.tval;
	try {
		get_threads(thr->tid);
	} catch (const std::out_of_range&) {
		send_packet(conn, "E01");
		return;
	}

	send_packet(conn, "OK");
}

void GDBServer::vCont(int conn, gdb_command_t *cmd) {
	gdb_vcont_t *vcont;
	const char *stop_reason = NULL;

	vcont = cmd->v.vval;
	for (vcont = cmd->v.vval; vcont; vcont = vcont->next) {
		if (vcont->action != 'c')
			throw std::invalid_argument("Unimplemented vCont action"); /* TODO */

		std::vector<debugable *> selected_harts;
		try {
			selected_harts = run_threads(vcont->thread.tid);
		} catch (const std::out_of_range&) {
			send_packet(conn, "E01");
			return;
		}

		for (debugable *hart : selected_harts) {
			switch (hart->status) {
			case CoreExecStatus::HitBreakpoint:
				stop_reason = "S05";
				break;
			case CoreExecStatus::Terminated:
				stop_reason = "S03";
				break;
			case CoreExecStatus::Runnable:
				continue;
			}

			/* mark runnable again */
			hart->status = CoreExecStatus::Runnable;
		}
	}

	assert(stop_reason);
	send_packet(conn, stop_reason);
}

void GDBServer::vContSupported(int conn, gdb_command_t *cmd) {
	// We need to support both c and C otherwise GDB doesn't use vCont
	// This is documented in the remote_vcont_probe function in the GDB source.
	send_packet(conn, "vCont;c;C");
}

void GDBServer::removeBreakpoint(int conn, gdb_command_t *cmd) {
	gdb_breakpoint_t *bpoint;

	bpoint = &cmd->v.bval;
	if (bpoint->type != GDB_ZKIND_SOFT) {
		send_packet(conn, ""); /* not supported */
		return;
	}

	auto fn = [bpoint] (debugable *hart) {
		hart->breakpoints.erase(bpoint->address);
	};

	exec_thread(fn);
	send_packet(conn, "OK");
}

void GDBServer::setBreakpoint(int conn, gdb_command_t *cmd) {
	gdb_breakpoint_t *bpoint;

	bpoint = &cmd->v.bval;
	if (bpoint->type != GDB_ZKIND_SOFT) {
		send_packet(conn, ""); /* not supported */
		return;
	}

	auto fn = [bpoint] (debugable *hart) {
		hart->breakpoints.insert(bpoint->address);
	};

	exec_thread(fn);
	send_packet(conn, "OK");
}
