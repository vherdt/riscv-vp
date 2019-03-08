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


bool DebugMemoryInterface::_do_transaction(tlm::tlm_command cmd, uint64_t addr, uint8_t *data, unsigned num_bytes) {
	tlm::tlm_generic_payload trans;
	trans.set_command(cmd);
	trans.set_address(addr);
	trans.set_data_ptr(data);
	trans.set_data_length(num_bytes);
	trans.set_response_status(tlm::TLM_OK_RESPONSE);

	isock->transport_dbg(trans);

	return !trans.is_response_error();
}

std::string DebugMemoryInterface::read_memory(uint64_t start, unsigned nbytes) {
	std::stringstream stream;
	std::vector<uint8_t> buf(nbytes);

	bool ok = _do_transaction(tlm::TLM_READ_COMMAND, start, buf.data(), buf.size());
	if (ok) {
		stream << std::setfill('0') << std::hex;
		for (unsigned i = 0; i < nbytes; ++i) {
			uint8_t byte = buf[i];
			stream << std::setw(2) << (unsigned)byte;
		}
	} else {
		std::cout << "WARNING: debug read transaction to address=" << start << " with size=" << nbytes << " failed -> return zero memory." << std::endl;
		for (unsigned i = 0; i < nbytes; ++i) {
			stream << "00";
		}
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

	bool ok = _do_transaction(tlm::TLM_WRITE_COMMAND, start, buf.data(), buf.size());
	if (!ok) {
		std::cout << "WARNING: debug write transaction to address=" << start << " with size=" << nbytes << " failed -> ignore write access." << std::endl;
	}
}


std::string DebugCoreRunner::read_memory(unsigned start, int nbytes) {
	assert(nbytes > 0);
	return dbg_mem_if->read_memory(start, boost::lexical_cast<unsigned>(nbytes));
}

void DebugCoreRunner::write_memory(unsigned start, int nbytes, const std::string &data) {
	assert(nbytes > 0);
    dbg_mem_if->write_memory(start, boost::lexical_cast<unsigned>(nbytes), data);
}


// reasons for halt as defined in: include/gdb/signals.h
// NOTE: this is only an excerpt, more signals are defined, please note the numbers might be not up to date ...
enum target_signal {
	TARGET_SIGNAL_FIRST = 0,
	TARGET_SIGNAL_HUP = 1,
	TARGET_SIGNAL_INT = 2,
	TARGET_SIGNAL_QUIT = 3,
	TARGET_SIGNAL_ILL = 4,
	TARGET_SIGNAL_TRAP = 5,
	TARGET_SIGNAL_ABRT = 6,
	TARGET_SIGNAL_EMT = 7,
	TARGET_SIGNAL_FPE = 8,
	TARGET_SIGNAL_KILL = 9,
	TARGET_SIGNAL_BUS = 10,
	TARGET_SIGNAL_SEGV = 11,
};

char nibble_to_hex[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

std::string compute_checksum_string(const std::string &msg) {
	unsigned sum = 0;
	for (auto c : msg) {
		sum += unsigned(c);
	}
	sum = sum % 256;

	char low = nibble_to_hex[sum & 0xf];
	char high = nibble_to_hex[(sum & (0xf << 4)) >> 4];

	return {high, low};
}

std::string DebugCoreRunner::receive_packet(int conn) {
	int nbytes = ::recv(conn, iobuf, bufsize, 0);
	assert(nbytes <= (long)bufsize);

	// std::cout << "recv: " << buffer << std::endl;

	if (nbytes == 0) {
		return "";
	} else if (nbytes == 1) {
		assert(iobuf[0] == '+');
		return std::string("+");
	} else {
		// 1) find $
		// 2) find #
		// 3) assert that two chars follow #

		char *start = strchr(iobuf, '$');
		char *end = strchr(iobuf, '#');

		assert(start && end);
		assert(start < end);
		assert(end < (iobuf + bufsize));
		assert(strnlen(end, 3) == 3);

		std::string message(start + 1, end - (start + 1));

		{
			std::string local_checksum = compute_checksum_string(message);
			std::string recv_checksum(end + 1, 2);
			assert(local_checksum == recv_checksum);
		}

		return message;
	}
}

void DebugCoreRunner::send_packet(int conn, const std::string &msg) {
	std::string frame = "+$" + msg + "#" + compute_checksum_string(msg);
	// std::cout << "send: " << frame << std::endl;

	assert(frame.size() < (long)bufsize);

	memcpy(iobuf, frame.c_str(), frame.size());

	int nbytes = ::send(conn, iobuf, frame.size(), 0);
	assert(nbytes == (long)frame.size());
}

void send_raw(int conn, const std::string &msg) {
	int nbytes = ::send(conn, msg.c_str(), msg.size(), 0);
	assert(nbytes == (long)msg.size());
}

struct memory_access_t {
	unsigned start;
	int nbytes;
};

memory_access_t parse_memory_access(const std::string &msg) {
	assert(msg[0] == 'm' || msg[0] == 'M');

	const char *comma_pos = strchr(msg.c_str(), ',');
	assert(comma_pos);

	const char *second = comma_pos + 1;
	const char *first = msg.c_str() + 1;

	assert(first < second);
	assert(second < (msg.c_str() + msg.size()));

	auto start = boost::lexical_cast<unsigned>(strtol(first, 0, 16));
	auto nbytes = boost::lexical_cast<int>(strtol(second, 0, 16));

	memory_access_t ans = {start, nbytes};
	return ans;
}

struct breakpoint_t {
	unsigned addr;
	int kind;
};

breakpoint_t parse_breakpoint(const std::string &msg) {
	// ‘Z0,addr,kind[;cond_list…][;cmds:persist,cmd_list…]’
	assert(msg[0] == 'Z' || msg[0] == 'z');
	assert(msg[1] == '0' || msg[1] == '1');

	std::vector<std::string> strs;
	boost::split(strs, msg, boost::is_any_of(","));

	unsigned addr = std::stoul(strs[1], nullptr, 16);
	int kind = std::stoi(strs[2], nullptr, 10);

	return {addr, kind};
}

bool is_any_watchpoint_or_hw_breakpoint(const std::string &msg) {
	bool is_hw_breakpoint = boost::starts_with(msg, "Z1") || boost::starts_with(msg, "z1");
	bool is_read_watchpoint = boost::starts_with(msg, "Z2") || boost::starts_with(msg, "z2");
	bool is_write_watchpoint = boost::starts_with(msg, "Z3") || boost::starts_with(msg, "z3");
	bool is_access_watchpoint = boost::starts_with(msg, "Z4") || boost::starts_with(msg, "z4");
	return is_hw_breakpoint || is_read_watchpoint || is_write_watchpoint || is_access_watchpoint;
}

uint32_t swap_byte_order(uint32_t n) {
	return htonl(n);
}

uint64_t swap_byte_order(uint64_t n) {
    return bswap_64(n);
}


void DebugCoreRunner::handle_gdb_loop(int conn) {
	while (true) {
		std::string msg = receive_packet(conn);
		if (msg.size() == 0) {
			std::cout << "remote connection seems to be closed, terminating ..." << std::endl;
			break;
		} else if (msg == "+") {
			// NOTE: just ignore this message, nothing to do in this case
		} else if (boost::starts_with(msg, "qSupported")) {
			send_packet(conn, "PacketSize=1024");
		} else if (msg == "vMustReplyEmpty") {
			send_packet(conn, "");
		} else if (msg == "Hg0") {
			send_packet(conn, "");
		} else if (msg == "Hc0") {
			send_packet(conn, "");
		} else if (msg == "qTStatus") {
			send_packet(conn, "");
		} else if (msg == "?") {
			send_packet(conn, "S05");
		} else if (msg == "qfThreadInfo") {
			send_packet(conn, "");
		} else if (boost::starts_with(msg, "qL")) {
			send_packet(conn, "");
		} else if (msg == "Hc-1") {
			send_packet(conn, "");
		} else if (msg == "qC") {
			send_packet(conn, "");
		} else if (msg == "qAttached") {
			send_packet(conn, "0");  // 0 process started, 1 attached to process
		} else if (msg == "g") {
			// data for all register using two digits per byte
			std::stringstream stream;
			stream << std::setfill('0') << std::hex;
			for (uint64_t v : regfile.regs) {
				stream << std::setw(16) << bswap_64(v); //swap_byte_order(v);
			}
			send_packet(conn, stream.str());
		} else if (boost::starts_with(msg, "p")) {
			long n = strtol(msg.c_str() + 1, 0, 16);
			assert(n >= 0);

			uint64_t reg_value;
			if (n < 32) {
				reg_value = regfile[n];
			} else if (n == 32) {
				reg_value = core.pc;
			} else {
				// see: https://github.com/riscv/riscv-gnu-toolchain/issues/217
				// risc-v register 834
				try {
					reg_value = core.get_csr_value(n - 65);
				} catch (SimulationTrap &e) {
					reg_value = 0;  // return zero for unsupported CSRs
				}
			}

			std::stringstream stream;
			stream << std::setfill('0') << std::hex << std::setw(16) << bswap_64(reg_value); //swap_byte_order(reg_value);
			send_packet(conn, stream.str());
		} else if (boost::starts_with(msg, "m")) {
			memory_access_t m = parse_memory_access(msg);
			std::string ans = read_memory(m.start, m.nbytes);
			send_packet(conn, ans);
		} else if (boost::starts_with(msg, "M")) {
			memory_access_t m = parse_memory_access(msg);
			std::string data = msg.substr(msg.find(":") + 1);
			write_memory(m.start, m.nbytes, data);
			send_packet(conn, "OK");
		} else if (boost::starts_with(msg, "X")) {
			send_packet(conn, "");  // binary data unsupported, gdb will send
			                        // text based message M
		} else if (msg == "qOffsets") {
			// NOTE: seems to be additional offsets wrt. the exec. elf file
			send_packet(conn, "Text=0;Data=0;Bss=0");
		} else if (msg == "qSymbol::") {
			send_packet(conn, "OK");
		} else if (msg == "vCont?") {
			send_packet(conn, "vCont;cs");
		} else if (msg == "c") {
			try {
				core.run();
				if (core.status == CoreExecStatus::HitBreakpoint) {
					send_packet(conn, "S05");
					core.status = CoreExecStatus::Runnable;
				} else if (core.status == CoreExecStatus::Terminated) {
					send_packet(conn, "S03");
				} else {
					assert(false && "invalid core status (apparently still marked as runnable)");
				}
			} catch (std::exception &e) {
				send_packet(conn, "S04");
			}
		} else if (msg == "s") {
			core.run_step();
			if (core.status == CoreExecStatus::Terminated) {
				send_packet(conn, "S03");
			} else {
				core.status = CoreExecStatus::Runnable;
				send_packet(conn, "S05");
			}
		} else if (boost::starts_with(msg, "vKill")) {
			send_packet(conn, "OK");
			break;
		} else if (boost::starts_with(msg, "Z0")) {
			// set SW breakpoint:
			// ‘Z0,addr,kind[;cond_list…][;cmds:persist,cmd_list…]’
			breakpoint_t s = parse_breakpoint(msg);
			// std::cout << "set breakpoint: " << s.addr << std::endl;
			core.breakpoints.insert(s.addr);
			send_packet(conn, "OK");
		} else if (boost::starts_with(msg, "z0")) {
			// remove SW breakpoint: ‘z0,addr,kind’
			breakpoint_t s = parse_breakpoint(msg);
			core.breakpoints.erase(s.addr);
			send_packet(conn, "OK");
		} else if (is_any_watchpoint_or_hw_breakpoint(msg)) {
			//NOTE: empty string means unsupported here, use OK to say it is supported
			send_packet(conn, "");
		} else {
			std::cout << "unsupported message '" << msg << "' detected, terminating ..." << std::endl;
			break;
		}
	}
}

void DebugCoreRunner::run() {
	constexpr int MAX_CONNECTIONS = 1;

	//  Socket to talk to clients
	int sock = ::socket(AF_INET, SOCK_STREAM, 0);

	int optval = 1;
	int ans = ::setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
	assert(ans == 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(5005);

	ans = ::bind(sock, (struct sockaddr *)&addr, sizeof(addr));
	assert(ans == 0);

	ans = ::listen(sock, MAX_CONNECTIONS);
	assert(ans == 0);

	socklen_t len = sizeof(addr);
	int conn = ::accept(sock, (struct sockaddr *)&addr, &len);
	assert(conn >= 0);

	handle_gdb_loop(conn);

	::close(sock);

	sc_core::sc_stop();
}
