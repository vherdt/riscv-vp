#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "?", &GDBServer::haltReason },
	{ "H", &GDBServer::setThread },
	{ "qSupported", &GDBServer::qSupported },
};

void GDBServer::haltReason(int conn, gdb_command_t *cmd) {
	// If n is a recognized stop reason […]. The aa should be
	// ‘05’, the trap signal. At most one stop reason should be
	// present.

	// TODO: Only send create conditionally.
	send_packet(conn, "T05create:;");
}

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	gdb_cmd_h_t *hcmd;

	hcmd = &cmd->v.hcmd;
	thread_ops[hcmd->op] = hcmd->id.tid;

	send_packet(conn, "OK");
}

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, "multiprocess+");
};
