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

/* Character used to introduce new packets */
#define GDB_DELIM '$'

/* Characters used for acknowledgments */
#define GDB_ACKS "+-"

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
	char *ptr, *line;
	size_t llen;

	line = NULL;
	llen = 0;

	while (getdelim(&line, &llen, GDB_DELIM, stream) != -1) {
		if ((ptr = strchr(line, GDB_DELIM)))
			*ptr = '\0';

		/* Acknowledgments are useless over TCP (disabled later) */
		if (strlen(line) == 1 && strchr(GDB_ACKS, *line))
			continue;

		printf("%s: received command: %s\n", __func__, line);
	}

	free(line);
	if (ferror(stream))
		err(1, "getdelim failed");
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
