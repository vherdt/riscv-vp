#include <assert.h>

#include "debug.h"
#include "core_defs.h"
#include "gdb_server.h"
#include "register_format.h"
#include "protocol/protocol.h"

enum {
	GDB_PC_REG = 32,
};

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "?", &GDBServer::haltReason },
	{ "g", &GDBServer::getRegisters },
	{ "H", &GDBServer::setThread },
	{ "p", &GDBServer::readRegister },
	{ "qAttached", &GDBServer::qAttached },
	{ "qSupported", &GDBServer::qSupported },
	{ "qfThreadInfo", &GDBServer::threadInfo },
	{ "qsThreadInfo", &GDBServer::threadInfoEnd },
};

void GDBServer::haltReason(int conn, gdb_command_t *cmd) {
	// If n is a recognized stop reason […]. The aa should be
	// ‘05’, the trap signal. At most one stop reason should be
	// present.

	// TODO: Only send create conditionally.
	send_packet(conn, "T05create:;");
}

void GDBServer::getRegisters(int conn, gdb_command_t *cmd) {
	int thread;

	try {
		thread = thread_ops.at('g');
	} catch (const std::system_error& e) {
		thread = GDB_THREAD_ALL;
	}

	if (thread == GDB_THREAD_ANY)
		thread = 1;

	auto fn = [this, conn] (debugable *hart) {
		/* TODO: do not hardcore architecture */
		RegisterFormater formatter(RV64);

		for (int64_t v : hart->get_registers())
			formatter.formatRegister(v);

		this->send_packet(conn, formatter.str().c_str());
	};

	assert(thread >= 0);
	if (thread == GDB_THREAD_ALL) {
		for (debugable *hart : harts)
			fn(hart);
	} else {
		assert(thread >= 1);
		fn(harts.at(thread - 1));
	}
}

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	gdb_cmd_h_t *hcmd;

	hcmd = &cmd->v.hcmd;
	thread_ops[hcmd->op] = hcmd->id.tid;

	send_packet(conn, "OK");
}

void GDBServer::readRegister(int conn, gdb_command_t *cmd) {
	int reg;
	int64_t regval;
	debugable *hart;
	RegisterFormater formatter(RV64);

	/* TODO: How should the hart be determined? */
	hart = harts[0];

	/* TODO: add get_register method to debugable */
	std::vector<int64_t> regs = hart->get_registers();

	reg = cmd->v.ival;
	if (reg == GDB_PC_REG) {
		regval = hart->get_program_counter();
	} else {
		regval = regs.at(reg); /* TODO: might be out-of-bounds */
	}
	/* TODO: handle CSRs? */

	formatter.formatRegister(regval);
	send_packet(conn, formatter.str().c_str());
}

void GDBServer::threadInfo(int conn, gdb_command_t *cmd) {
	std::string thrlist = "m";

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
	send_packet(conn, "multiprocess+");
}
