/*
 * gpio-server.cpp
 *
 *  Created on: 5 Nov 2018
 *      Author: dwd
 */

#include "gpio-server.hpp"
#include <iostream>

using namespace std;

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

	gpio.startListening();
}
