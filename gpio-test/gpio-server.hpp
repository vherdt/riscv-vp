/*
 * gpio-server.hpp
 *
 *  Created on: 7 Nov 2018
 *      Author: dwd
 */

#pragma once

#include "gpio.hpp"
#include <functional>

class GpioServer: public Gpio
{
	int fd;
	volatile bool stop;
	std::function<void(uint8_t bit)> fun;
	void handleConnection(int conn);
public:
	GpioServer();
	~GpioServer();
	bool setupConnection(const char* port);
	void quit();
	void registerOnChange(std::function<void(uint8_t bit)> fun);
	void startListening();
};

