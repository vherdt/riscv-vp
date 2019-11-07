#include <err.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <systemc>

#include "gdb_server.h"
#include "protocol/parser.h"

GDBServer::GDBServer(sc_core::sc_module_name name,
                     std::vector<debugable*> harts,
                     DebugMemoryInterface *mm,
                     uint16_t port) {
	create_sock(port);
	thr = std::thread(&GDBServer::serve, this);
	thr.detach();
}

void GDBServer::create_sock(uint16_t port) {
	struct sockaddr_in6 addr;
	int optval;

	sockfd = socket(AF_INET6, SOCK_STREAM, 0);
	if (sockfd == -1)
		throw std::system_error(errno, std::generic_category());

	optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1)
		goto err;

	optval = 0;
	if (setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) == -1)
		goto err;

	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_addr = in6addr_loopback;
	addr.sin6_port = htons(port);

	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) == -1)
		goto err;
	if (listen(sockfd, 0) == -1)
		goto err;

	return;
err:
	close(sockfd);
	throw std::system_error(errno, std::generic_category());
}

void GDBServer::dispatch(FILE *stream) {
	gdb_packet_t *pkt;

	while ((pkt = gdb_parse(stream))) {
		printf("%s: received packet { kind: %d, data: '%s', csum: 0x%c%c }\n",
		       __func__, pkt->kind, (pkt->data) ? pkt->data : "", pkt->csum[0], pkt->csum[1]);

		if (!gdb_is_valid(pkt))
			printf("\tchecksum is invalid\n");

		gdb_free_packet(pkt);
	}
}

void GDBServer::serve(void) {
	int conn;
	FILE *stream;

	for (;;) {
		if ((conn = accept(sockfd, NULL, NULL)) == -1) {
			warn("accept failed");
			continue;
		}

		if (!(stream = fdopen(conn, "r+")))
			throw std::system_error(errno, std::generic_category());

		dispatch(stream);
		fclose(stream);
	}
}
