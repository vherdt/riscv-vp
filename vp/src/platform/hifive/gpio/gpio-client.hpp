/*
 * gpio-client.hpp
 *
 *  Created on: 7 Nov 2018
 *      Author: dwd
 */

#pragma once

#include "gpiocommon.hpp"

class GpioClient : public GpioCommon {
	int fd;

   public:
	GpioClient();
	~GpioClient();
	bool setupConnection(const char* host, const char* port);
	bool update();
	bool setBit(uint8_t pos, Tristate val);
};
