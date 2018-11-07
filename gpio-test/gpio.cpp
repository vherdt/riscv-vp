/*
 * gpio.cpp
 *
 *  Created on: 7 Nov 2018
 *      Author: dwd
 */

#include "gpio.hpp"
#include <iostream>
#include <stdio.h>
#include <cstring>
#include <unistd.h>

using namespace std;

void hexPrint(char* buf, size_t size){
	for(uint16_t i = 0; i < size; i++)
	{
		printf("%2X ", buf[i]);
	}
	cout << endl;
}

void bitPrint(char* buf, size_t size)
{
	for(uint16_t byte = 0; byte < size; byte++)
	{
		for(int8_t bit = 7; bit >= 0; bit--)
		{
			printf("%c", buf[byte] & (1 << bit) ? '1' : '0');
		}
		printf(" ");
	}
	printf("\n");
}

void Gpio::printRequest(Request* req)
{
	switch(req->op)
	{
	case GET_BANK:
		cout << "GET BANK";
		break;
	case SET_BIT:
		cout << "SET BIT ";
		cout << to_string(req->setBit.pos) << " to ";
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
};

Gpio::Gpio()
{
	memset(&state, 0, sizeof(State));
}

void GpioServer::handleConnection(int fd)
{
	Request req;
	memset(&req, 0, sizeof(Request));
	int bytes;
	while((bytes = read(fd, &req, sizeof(Request))) > 0)
	{
		printRequest(&req);
		hexPrint(reinterpret_cast<char*>(&req), bytes);
		switch(req.op)
		{
		case GET_BANK:
			if (write(fd, &state.val, sizeof(Reg)) < sizeof(Reg))
			{
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
	cout << "client disconnected." << endl;
}

bool GpioClient::update(int fd)
{
	Request req;
	memset(&req, 0, sizeof(Request));
	req.op = GET_BANK;
	if(write(fd, &req, sizeof(Request)) < sizeof(Request))
	{
		cerr << "Error in write" << endl;
		return false;
	}
	if(read(fd, &state.val, sizeof(Reg)) < sizeof(Reg))
	{
		cerr << "Error in read" << endl;
		return false;
	}
	return true;
}

bool GpioClient::setBit(int fd, uint8_t pos, uint8_t tristate)
{
	Request req;
	memset(&req, 0, sizeof(Request));
	req.op = SET_BIT;
	req.setBit.pos = pos;
	req.setBit.tristate = tristate;

	if(write(fd, &req, sizeof(Request)) < sizeof(Request))
	{
		cerr << "Error in write" << endl;
		return false;
	}
	return true;
}



