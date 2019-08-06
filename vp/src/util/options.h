#pragma once

#include <functional>
#include <stdexcept>
#include <string>

template <typename T>
struct OptionValue {
	bool available = false;
	T value{};
	std::string option;

	bool finalize(std::function<T(const std::string &)> parser) {
		if (!option.empty()) {
			value = parser(option);
			available = true;
		}
		return available;
	}
};

unsigned long parse_ulong_option(const std::string &s) {
	bool is_hex = false;
	if (s.size() >= 2) {
		if ((s[0] == '0') && ((s[1] == 'x') || (s[1] == 'X')))
			is_hex = true;
	}

	try {
		if (is_hex)
			return stoul(s, 0, 16);
		return stoul(s);
	} catch (std::exception &e) {
		throw std::runtime_error(std::string("unable to parse option '") + s + "' into a number");
	}
}