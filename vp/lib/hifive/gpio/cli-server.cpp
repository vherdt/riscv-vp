/*
 * gpio-server.cpp
 *
 *  Created on: 5 Nov 2018
 *      Author: dwd
 */

#include <iostream>
#include <thread>
#include <functional>
#include <unistd.h>
#include <csignal>

#include "gpio-server.hpp"

using namespace std;

bool stop = false;

void signalHandler( int signum ) {
   cout << "Interrupt signal (" << signum << ") received.\n";

   if(stop)
	   exit(signum);
   stop = true;
   raise(SIGUSR1);	//this breaks wait in thread
}

void onChangeCallback(uint8_t bit)
{
	printf("Bit %d changed\n", bit);
}

int main(int argc, char* argv[])
{
	if(argc < 2)
	{
		cout << "usage: " << argv[0] << " port (e.g. 1339)" << endl;
		exit(-1);
	}

	GpioServer gpio;

	if(!gpio.setupConnection(argv[1]))
	{
		cerr << "cant set up server" << endl;
	}

	signal(SIGINT, signalHandler);

	gpio.registerOnChange(onChangeCallback);
	thread server(bind(&GpioServer::startListening, &gpio));

	while(!stop)
	{
		usleep(100000);
		if(!(gpio.state & (1 << 11)))
		{
			gpio.state <<= 1;
			if(!(gpio.state & 0xFF))
			{
				gpio.state = 1;
			}
		}
	}
	gpio.quit();
	server.join();
}
