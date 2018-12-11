#include "gdb_stub.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>


std::string debug_memory_mapping::read_memory(unsigned start, int nbytes) {
    assert (start >= 0);
    assert (nbytes > 0);

    uint32_t read_offset = boost::lexical_cast<uint32_t>(start);
    uint32_t read_size = boost::lexical_cast<uint32_t>(nbytes);

    std::stringstream stream;
    if(read_offset >= mem_offset)
    {
		assert ((read_offset + read_size) <= (mem_offset + mem_size));

		uint32_t local_offset = read_offset - mem_offset;
        stream << std::setfill('0') << std::hex;
        for (uint32_t i=0; i<read_size; ++i) {
            uint8_t byte = mem[local_offset+i];
            stream << std::setw(2) << (unsigned)byte;
        }
    }
    else
    {
        for (uint32_t i=0; i<read_size; ++i) {
            stream << "00";
        }
    }

    return stream.str();
}

std::string debug_memory_mapping::zero_memory(int nbytes) {
    assert (nbytes > 0);

    uint32_t read_size = boost::lexical_cast<uint32_t>(nbytes);

    std::stringstream stream;
    stream << std::setfill('0') << std::hex;
    for (uint32_t i=0; i<read_size; ++i) {
        stream << std::setw(2) << 0;
    }
    return stream.str();
}


void debug_memory_mapping::write_memory(unsigned start, int nbytes, const std::string &data) {
    assert (start >= 0);
    assert (nbytes > 0);
    assert (data.length() % 2 == 0);

    uint32_t write_offset = boost::lexical_cast<uint32_t>(start);
    uint32_t write_size = boost::lexical_cast<uint32_t>(nbytes);

    assert (write_offset >= mem_offset);
    assert ((write_offset + write_size) <= (mem_offset + mem_size));

    uint32_t local_offset = write_offset - mem_offset;

    for (unsigned i=0; i<write_size; ++i) {
        std::string bytes = data.substr(i*2, 2);
        uint8_t byte = (uint8_t)std::strtol(bytes.c_str(), NULL, 16);
        mem[local_offset+i] = byte;
    }
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


char nibble_to_hex[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

std::string compute_checksum_string(const std::string &msg) {
    unsigned sum = 0;
    for (auto c : msg) {
        sum += unsigned(c);
    }
    sum = sum % 256;

    char low  = nibble_to_hex[sum & 0xf];
    char high = nibble_to_hex[(sum & (0xf << 4)) >> 4];

    return {high, low};
}


std::string DebugCoreRunner::receive_packet(int conn) {
    int nbytes = ::recv(conn, iobuf, bufsize, 0);
    if(nbytes <= 0)
    {
    	std::cerr << "recv error" << strerror(errno) << std::endl;
    	return std::string("err");
    }
    assert (nbytes <= bufsize);

    //std::cout << "recv: " << buffer << std::endl;

    if (nbytes == 0) {
        return "";
    } else if (nbytes == 1) {
        assert (iobuf[0] == '+');
        return std::string("+");
    } else {
        //1) find $
        //2) find #
        //3) assert that two chars follow #

        char *start = strchr(iobuf, '$');
        char *end   = strchr(iobuf, '#');

        assert (start && end);
        assert (start < end);
        assert (end < (iobuf + bufsize));
        assert (strnlen(end, 3) == 3);

        std::string message(start+1, end-(start+1));

        {
            std::string local_checksum = compute_checksum_string(message);
            std::string recv_checksum(end+1, 2);
            assert (local_checksum == recv_checksum);
        }

        return message;
    }
}


void DebugCoreRunner::send_packet(int conn, const std::string &msg) {
    std::string frame = "+$" + msg + "#" + compute_checksum_string(msg);
    //std::cout << "send: " << frame << std::endl;

    assert (frame.size() < bufsize);

    memcpy(iobuf, frame.c_str(), frame.size());

    int nbytes = ::send(conn, iobuf, frame.size(), 0);
    assert (nbytes == frame.size());
}


void send_raw(int conn, const std::string &msg) {
    int nbytes = ::send(conn, msg.c_str(), msg.size(), 0);
    assert (nbytes == msg.size());
}


struct memory_access_t {
    unsigned start;
    int nbytes;
};

memory_access_t parse_memory_access(const std::string &msg) {
    assert (msg[0] == 'm' || msg[0] == 'M');

    const char *comma_pos = strchr(msg.c_str(), ',');
    assert (comma_pos);

    const char *second = comma_pos + 1;
    const char *first = msg.c_str() + 1;

    assert (first < second);
    assert (second < (msg.c_str()+msg.size()));

    auto start  = boost::lexical_cast<unsigned>(strtol(first, 0, 16));
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
    assert (msg[0] == 'Z' || msg[0] == 'z');
    assert (msg[1] == '0' || msg[1] == '1');

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


void DebugCoreRunner::handle_gdb_loop(int conn) {
    while (true) {
        std::string msg = receive_packet(conn);
        if (msg.size() == 0) {
            std::cout << "remote connection seems to be closed, terminating ..." << std::endl;
            break;
        } else if (msg == "+") {
            // NOTE: just ignore this message, nothing to do in this case
        }else if (msg == "err") {
            //error from receive packet. Ignore...
        	std::cerr << "Received packet unknown" << std::endl;
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
            send_packet(conn, "0");    // 0 process started, 1 attached to process
        } else if (msg == "g") {
            // data for all register using two digits per byte
            std::stringstream stream;
            stream << std::setfill('0') << std::hex;
            for (auto v : regfile.regs) {
                stream << std::setw(8) << swap_byte_order(v);
            }
            send_packet(conn, stream.str());
        } else if (boost::starts_with(msg, "p")) {
            long n = strtol(msg.c_str() + 1, 0, 16);
            assert (n >= 0);

            uint32_t reg_value;
            if (n < 32) {
                reg_value = regfile[n];
            } else if (n == 32) {
                reg_value = core.pc;
            } else {
                // see: https://github.com/riscv/riscv-gnu-toolchain/issues/217
                // risc-v register 834
                auto it = core.csrs.addr_to_csr.find(n - 65);
                if (it != core.csrs.addr_to_csr.end()) {
                    csr_base &reg = *(it->second);
                    reg_value = reg.unchecked_read();
                } else {
                    reg_value = 0; // return zero for unsupported CSRs
                }
            }

            std::stringstream stream;
            stream << std::setfill('0') << std::hex << std::setw(8) << swap_byte_order(reg_value);
            send_packet(conn, stream.str());
        } else if (boost::starts_with(msg, "m")) {
            memory_access_t m = parse_memory_access(msg);
            std::string ans;
        	//NOTE: reading from a memory mapped device is currently not supported (can implement a *debug-read* method on the bus or similar)
            if((m.start + m.nbytes) <= (memory.mem_offset + memory.mem_size))
            {
            	ans = memory.read_memory(m.start, m.nbytes);
			}
            else
			{	//NOTE: out of bound memory access
            	ans = memory.zero_memory(m.nbytes);
			}
            send_packet(conn, ans);
        } else if (boost::starts_with(msg, "M")) {
            memory_access_t m = parse_memory_access(msg);
            assert ((m.start + m.nbytes) <= (memory.mem_offset + memory.mem_size)); //NOTE: out of bound memory access
            std::string data = msg.substr(msg.find(":") + 1);
            memory.write_memory(m.start, m.nbytes, data);
            send_packet(conn, "OK");
        } else if (boost::starts_with(msg, "X")) {
            send_packet(conn, "");  // binary data unsupported, gdb will send text based message M
        } else if (msg == "qOffsets") {
            //NOTE: seems to be additional offsets wrt. the exec. elf file
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
                    assert (false && "invalid core status (apparently still marked as runnable)");
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
            // set SW breakpoint: ‘Z0,addr,kind[;cond_list…][;cmds:persist,cmd_list…]’
            breakpoint_t s = parse_breakpoint(msg);
            //std::cout << "set breakpoint: " << s.addr << std::endl;
            core.breakpoints.insert(s.addr);
            send_packet(conn, "OK");
        } else if (boost::starts_with(msg, "z0")) {
            // remove SW breakpoint: ‘z0,addr,kind’
            breakpoint_t s = parse_breakpoint(msg);
            core.breakpoints.erase(s.addr);
            send_packet(conn, "OK");
        } else if (is_any_watchpoint_or_hw_breakpoint(msg)) {
            send_packet(conn, "");  //NOTE: empty string means unsupported here, use OK to say it is supported
            break;
        } else {
            std::cout << "unsupported message detected, terminating ..." << std::endl;
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
    assert (ans == 0);

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(5005);

    ans = ::bind(sock, (struct sockaddr *)&addr, sizeof(addr));
    assert (ans == 0);

    ans = ::listen(sock, MAX_CONNECTIONS);
    assert (ans == 0);

    socklen_t len = sizeof(addr);
    int conn = ::accept(sock, (struct sockaddr *)&addr, &len);
    assert (conn >= 0);

    handle_gdb_loop(conn);

    ::close(sock);

    sc_core::sc_stop();
}
