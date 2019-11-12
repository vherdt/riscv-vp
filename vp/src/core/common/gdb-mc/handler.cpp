#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "qSupported", &GDBServer::qSupported },
	{ "vMustReplyEmpty", &GDBServer::vMustReplyEmpty },
};

void GDBServer::qSupported(int conn, gdb_command_t *cmd) {
	send_packet(conn, "stubfeature;multiprocess+");
};

void GDBServer::vMustReplyEmpty(int conn, gdb_command_t *cmd) {
	/* The correct reply to an unknown ‘v’ packet
	 * is to return the empty strings […] */
	send_packet(conn, "");
};
