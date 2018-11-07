/*
 * gpio-server.hpp
 *
 *  Created on: 7 Nov 2018
 *      Author: dwd
 */

#pragma once

#include "gpio.hpp"

class GpioServer: public Gpio
{
	int fd;
	volatile bool stop;
	void handleConnection(int conn);
public:
	GpioServer();
	~GpioServer();
	bool setupConnection(const char* port);
	void quit();
	void startListening();
};

