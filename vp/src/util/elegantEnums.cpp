/*
 * elegantEnums.hpp
 *
 *  Created on: 14 Mar 2019
 *      Author: dwd
 */

#include "elegantEnums.hpp"

std::vector<std::string> splitString(std::string str, char sep) {
	std::vector<std::string> vecString;
	std::string item;

	std::stringstream stringStream(str);

	while (std::getline(stringStream, item, sep)) {
		vecString.push_back(item);
	}

	return vecString;
}
