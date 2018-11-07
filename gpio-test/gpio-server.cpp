/*
 * gpio-server.cpp
 *
 *  Created on: 5 Nov 2018
 *      Author: dwd
 */

#include "gpio-server.hpp"

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

using namespace std;

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

GpioServer::GpioServer() : fd(-1), stop(false)
{}

GpioServer::~GpioServer()
{
	if(fd >= 0)
	{
		close(fd);
	}
}

bool GpioServer::setupConnection(const char* port)
{
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address information
    socklen_t sin_size;
    struct sigaction sa;
    int yes=1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            return false;
        }

        if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        return false;
    }

    if (listen(fd, 1) == -1) {
        perror("listen");
        return false;
    }

    GpioServer gpio;
    printf("server: waiting for connections...\n");

    return true;
}

void GpioServer::startListening()
{
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size = sizeof their_addr;
	char s[INET6_ADDRSTRLEN];

	while(!stop)	//this would block a bit
	{
		int new_fd = accept(fd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			return;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s);
		printf("server: got connection from %s\n", s);
		handleConnection(new_fd);
		close(new_fd);
	}
}

void GpioServer::handleConnection(int conn)
{
	Request req;
	memset(&req, 0, sizeof(Request));
	int bytes;
	while((bytes = read(conn, &req, sizeof(Request))) == sizeof(Request))
	{
		printRequest(&req);
		hexPrint(reinterpret_cast<char*>(&req), bytes);
		switch(req.op)
		{
		case GET_BANK:
			if (write(conn, &state.val, sizeof(Reg)) != sizeof(Reg))
			{
				cerr << "could not write answer" << endl;
				return;
			}
			break;
		case SET_BIT:
			if(req.setBit.pos > 63)
			{
				cerr << "invalid request" << endl;
				return;
			}
			if(req.setBit.tristate == 0)
				state.val &= ~(1l << req.setBit.pos);
			else if(req.setBit.tristate == 1)
				state.val |= 1l << req.setBit.pos;
			else if(req.setBit.tristate == 2)
				cout << "set bit " << req.setBit.pos << " unset" << endl;
			else
			{
				cerr << "invalid request" << endl;
				return;
			}
			break;
		default:
			cerr << "invalid request" << endl;
			return;
		}
	}
	cout << "client disconnected. (" << bytes << ")" << endl;
}
