#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "H", &GDBServer::setThread },
	{ "qSupported", &GDBServer::qSupported },
};

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	gdb_cmd_h_t *hcmd;

	hcmd = &cmd->v.hcmd;
	thread_ops[hcmd->op] = hcmd->id.tid;

	send_packet(conn, "OK");
}

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, "multiprocess+");
};
