#include "gdb_server.h"
#include "protocol/protocol.h"

std::map<std::string, GDBServer::packet_handler> handlers {
	{ "qSupported", &GDBServer::qSupported },
};

void GDBServer::qSupported(int conn, gdb_packet_t *pkt) {
	/* TODO */
	std::cout << "qSupported" << std::endl;
};
