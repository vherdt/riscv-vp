#include <err.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <systemc>

#include "gdb_server.h"
#include "protocol/protocol.h"

/* TODO: move this header to common? */
#include <platform/hifive/async_event.h>

extern std::map<std::string, GDBServer::packet_handler> handlers;

GDBServer::GDBServer(sc_core::sc_module_name name,
                     std::vector<debugable*> dharts,
                     DebugMemoryInterface *mm,
                     uint16_t port) {
	if (dharts.size() <= 0)
		throw std::invalid_argument("no harts specified");

	arch = dharts.at(0)->get_architecture(); // assuming all harts use the same
	harts = dharts;
	prevpkt = NULL;
	create_sock(port);

	SC_METHOD(run);
	sensitive << asyncEvent;
	dont_initialize();

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

void GDBServer::exec_thread(thread_func fn, char op) {
	int thread;

	try {
		thread = thread_ops.at(op);
	} catch (const std::system_error& e) {
		thread = GDB_THREAD_ANY;
	}

	if (thread == GDB_THREAD_ANY)
		thread = 1;

	assert(thread >= 0);
	if (thread == GDB_THREAD_ALL) {
		for (debugable *hart : harts)
			fn(hart);
	} else {
		assert(thread >= 1);
		fn(harts.at(thread - 1));
	}
}

void GDBServer::writeall(int fd, char *data, size_t len) {
	ssize_t ret, w;

	w = 0;
	do {
		assert(len >= (size_t)w);
		ret = write(fd, &data[w], len - (size_t)w);
		if (ret < 0)
			throw std::system_error(errno, std::generic_category());

		w += ret;
	} while ((size_t)w < len);
}

void GDBServer::send_packet(int conn, const char *data, gdb_kind_t kind) {
	char *serialized;

	serialized = gdb_serialize(kind, data);

	printf("serialized '%s'\n", serialized);

	try {
		writeall(conn, serialized, strlen(serialized));
	} catch (const std::system_error& e) {
		warnx("writeall failed: %s", e.what());
		goto ret;
	}

	/* acknowledgment are only used for GDB packets */
	if (kind != GDB_KIND_PACKET)
		goto ret;

	free(prevpkt);
	if (!(prevpkt = strdup(serialized))) {
		prevpkt = NULL;
		free(serialized);
		throw std::system_error(errno, std::generic_category());
	}

ret:
	free(serialized);
}

void GDBServer::retransmit(int conn) {
	if (!prevpkt)
		return;

	try {
		writeall(conn, prevpkt, strlen(prevpkt));
	} catch (const std::system_error& e) {
		warnx("writeall failed: %s", e.what());
	}

	/* memory for prevpkt is freed on next successfull
	 * packet transmit in the send_packet function */
}

void GDBServer::dispatch(int conn) {
	FILE *stream;
	gdb_packet_t *pkt;

	if (!(stream = fdopen(conn, "r")))
		throw std::system_error(errno, std::generic_category());

	for (;;) {
		mtx.lock();
		if (!(pkt = gdb_parse_pkt(stream))) {
			mtx.unlock();
			break;
		}

		printf("%s: received packet { kind: %d, data: '%s', csum: 0x%c%c }\n",
		       __func__, pkt->kind, (pkt->data) ? pkt->data : "", pkt->csum[0], pkt->csum[1]);

		pktq.push(std::make_tuple(conn, pkt));
		asyncEvent.notify();

		/* further processing is performed in the SystemC run()
		 * thread which interacts with the ISS objects and
		 * unlocks the mutex after finishing all operations. */
	}

	fclose(stream);
}

void GDBServer::serve(void) {
	int conn;
	FILE *stream;

	for (;;) {
		if ((conn = accept(sockfd, NULL, NULL)) == -1) {
			warn("accept failed");
			continue;
		}

		dispatch(conn);
		close(conn);
	}
}

void GDBServer::run(void) {
	int conn;
	gdb_command_t *cmd;
	gdb_packet_t *pkt;
	packet_handler handler;

	auto ctx = pktq.front();
	std::tie (conn, pkt) = ctx;

	switch (pkt->kind) {
	case GDB_KIND_NACK:
		retransmit(conn);
		/* fall through */
	case GDB_KIND_ACK:
		goto next1;
	}

	if (!(cmd = gdb_parse_cmd(pkt))) {
		send_packet(conn, NULL, GDB_KIND_NACK);
		goto next1;
	}

	send_packet(conn, NULL, GDB_KIND_ACK);
	try {
		handler = handlers.at(cmd->name);
	} catch (const std::out_of_range&) {
		// For any command not supported by the stub, an
		// empty response (‘$#00’) should be returned.
		send_packet(conn, "");
		goto next2;
	}

	(this->*handler)(conn, cmd);

next2:
	gdb_free_cmd(cmd);
next1:
	gdb_free_packet(pkt);
	pktq.pop();
	mtx.unlock();
}
