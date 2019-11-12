#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "H", &GDBServer::setThread },
	{ "qSupported", &GDBServer::qSupported },
	{ "vMustReplyEmpty", &GDBServer::vMustReplyEmpty },
};

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	printf("setThread()\n");
	printf("type: %d\n", cmd->type);
}

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, "stubfeature;multiprocess+");
};

void GDBServer::vMustReplyEmpty(int conn, gdb_command_t *cmd) {
	/* The correct reply to an unknown ‘v’ packet
	 * is to return the empty strings […] */
	send_packet(conn, "");
};
