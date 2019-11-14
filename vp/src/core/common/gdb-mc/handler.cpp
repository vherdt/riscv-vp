#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "H", &GDBServer::setThread },
	{ "qSupported", &GDBServer::qSupported },
};

void GDBServer::setThread(int conn, gdb_command_t *cmd) {
	printf("setThread()\n");
	printf("type: %d\n", cmd->type);
}

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, "stubfeature;multiprocess+");
};
