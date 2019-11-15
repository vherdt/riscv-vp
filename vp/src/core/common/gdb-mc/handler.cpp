#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "?", &GDBServer::haltReason },
	{ "H", &GDBServer::setThread },
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

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	gdb_cmd_h_t *hcmd;

	hcmd = &cmd->v.hcmd;
	thread_ops[hcmd->op] = hcmd->id.tid;

	send_packet(conn, "OK");
}

void GDBServer::threadInfo(int conn, gdb_command_t *cmd) {
	std::string thrlist = "m";

	for (size_t i = 0; i < harts.size(); i++) {
		thrlist += std::to_string(i + 1);
		if (i + 1 < harts.size())
			thrlist += ",";
	}

	std::cout << "thrlist: " << thrlist << std::endl;
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

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, "multiprocess+");
}
