#include <assert.h>

#include "debug.h"
#include "gdb_server.h"
#include "register_format.h"
#include "protocol/protocol.h"

enum {
	GDB_PKTSIZ = 4096,
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
	{ "vCont", &GDBServer::vCont },
	{ "vCont?", &GDBServer::vContSupported },
};

void GDBServer::haltReason(int conn, gdb_command_t *cmd) {
	// If n is a recognized stop reason […]. The aa should be
	// ‘05’, the trap signal. At most one stop reason should be
	// present.

	// TODO: Only send create conditionally.
	send_packet(conn, "T05create:;");
}

void GDBServer::getRegisters(int conn, gdb_command_t *cmd) {
	auto fn = [this, conn] (debugable *hart) {
		RegisterFormater formatter(arch);

		for (int64_t v : hart->get_registers())
			formatter.formatRegister(v);

		this->send_packet(conn, formatter.str().c_str());
	};

	exec_thread(fn);
}

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	gdb_cmd_h_t *hcmd;

	hcmd = &cmd->v.hcmd;
	thread_ops[hcmd->op] = hcmd->id.tid;

	send_packet(conn, "OK");
}

void GDBServer::readRegister(int conn, gdb_command_t *cmd) {
	int reg;
	debugable *hart;

	reg = cmd->v.ival;
	auto fn = [this, reg, conn] (debugable *hart) {
		int64_t regval;
		RegisterFormater formatter(arch);

		if (reg == GDB_PC_REG) {
			regval = hart->get_program_counter();
		} else {
			/* TODO: add get_register method to debugable */
			std::vector<int64_t> regs = hart->get_registers();
			if (reg >= regs.size()) {
				send_packet(conn, "E01"); /* TODO: Only send error once */
				return;
			}
			regval = regs.at(reg);
		}

		/* TODO: handle CSRs? */

		formatter.formatRegister(regval);
		send_packet(conn, formatter.str().c_str());
	};

	exec_thread(fn);
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

void GDBServer::vCont(int conn, gdb_command_t *cmd) {
	_gdb_vcont_t *vcont;

	vcont = cmd->v.vval;
	for (vcont = cmd->v.vval; vcont; vcont = vcont->next)
		printf("%s: vcont->action = %c\n", __func__, vcont->action);

	/* TODO: implement the actual logic */
	send_packet(conn, "S00");
}

void GDBServer::vContSupported(int conn, gdb_command_t *cmd) {
	// We need to support both c and C otherwise GDB doesn't use vCont
	// This is documented in the remote_vcont_probe function in the GDB source.
	send_packet(conn, "vCont;c;C;s;S;t");
}
