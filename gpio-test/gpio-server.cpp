/*
 * gpio-server.cpp
 *
 *  Created on: 5 Nov 2018
 *      Author: dwd
 */

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

void hexPrint(char* buf, uint16_t size)
{
	for(uint16_t i = 0; i < size; i++)
	{
		printf("%2X ", buf[i]);
	}
	cout << endl;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

class Gpio
{
	typedef uint64_t Reg;
	struct State
	{
		Reg val;
		uint16_t interrupt_route[64];
	} state;

	enum Operation : uint8_t
	{
		GET_BANK = 1,
		SET_BIT
	};

	struct Request
	{
		Operation op;
		union
		{
			struct
			{
				uint8_t pos : 6;		//Todo: Packed
				uint8_t tristate : 2;
			} setBit;
		};
	};

	void printRequest(Request* req)
	{
		switch(req->op)
		{
		case GET_BANK:
			cout << "GET BANK";
			break;
		case SET_BIT:
			cout << "SET BIT ";
			cout << req->setBit.pos << " to ";
			switch(req->setBit.tristate)
			{
			case 0:
				cout << "LOW";
				break;
			case 1:
				cout << "HIGH";
				break;
			case 2:
				cout << "UNSET";
				break;
			default:
				cout << "INVALID";
			}
			break;
		default:
			cout << "INVALID";
		}
		cout << endl;
	}

public:
	Gpio()
	{
		memset(&state, 0, sizeof(State));
	}
	void handleConnection(int fd)
	{
		char buf[sizeof(Request)];
	    int bytes;
	    while((bytes = read(fd, buf, sizeof(Request))) > 0)
		{
	        hexPrint(buf, bytes);
	        printRequest(reinterpret_cast<Request*>(buf));
	    	if (write(fd, "OK", 3) == -1)
	    		break;
		}
	    cout << "client disconnected." << endl;
	}
};

int main(void)
{
    int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
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

    if ((rv = getaddrinfo(NULL, "1339", &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            perror("setsockopt");
            exit(1);
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("server: bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo); // all done with this structure

    if (p == NULL)  {
        fprintf(stderr, "server: failed to bind\n");
        exit(1);
    }

    if (listen(sockfd, 1) == -1) {
        perror("listen");
        exit(1);
    }

    Gpio gpio;
    printf("server: waiting for connections...\n");

    while(1) {  // main accept() loop
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("accept");
            continue;
        }

        inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr),
            s, sizeof s);
        printf("server: got connection from %s\n", s);
        gpio.handleConnection(new_fd);
		close(new_fd);
    }

    return 0;
}
