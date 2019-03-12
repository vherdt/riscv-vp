#include "gdb_stub.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <byteswap.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>


unsigned DebugMemoryInterface::_do_dbg_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes) {
	tlm::tlm_generic_payload trans;
	trans.set_command(cmd);
	trans.set_address(addr);
	trans.set_data_ptr(data);
	trans.set_data_length(num_bytes);
	trans.set_response_status(tlm::TLM_OK_RESPONSE);

	unsigned nbytes = isock->transport_dbg(trans);

	if (trans.is_response_error()) {
		std::cout << "WARNING: debug transaction to address=" << addr << " with size=" << nbytes << " and command=" << cmd << "failed." << std::endl;
		assert (false);
	}

	return nbytes;
}

std::string DebugMemoryInterface::read_memory(uint64_t start, unsigned nbytes) {
	std::vector<uint8_t> buf(nbytes); //NOTE: every element is zero-initialized by default

	unsigned nbytes_read = _do_dbg_transaction(tlm::TLM_READ_COMMAND, start, buf.data(), buf.size());
	assert (nbytes_read == nbytes && "not all bytes read");

	std::stringstream stream;
	stream << std::setfill('0') << std::hex;
	for (uint8_t byte : buf) {
		stream << std::setw(2) << (unsigned)byte;
	}

	return stream.str();
}

void DebugMemoryInterface::write_memory(uint64_t start, unsigned nbytes, const std::string &data) {
	assert(data.length() % 2 == 0);

	std::vector<uint8_t> buf(nbytes / 2);

	for (uint64_t i = 0; i < nbytes; ++i) {
		std::string bytes = data.substr(i*2, 2);
		uint8_t byte = (uint8_t)std::strtol(bytes.c_str(), NULL, 16);
		buf.at(i) = byte;
	}

	unsigned nbytes_write = _do_dbg_transaction(tlm::TLM_WRITE_COMMAND, start, buf.data(), buf.size());
	assert (nbytes_write == nbytes && "not all data written");
}