#include "options.h"

#include <iostream>
#include <unistd.h>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

Options::Options(void) {
	// clang-format off
	add_options()
		("help", "produce help message")
		("intercept-syscalls", po::bool_switch(&intercept_syscalls), "directly intercept and handle syscalls in the ISS")
		("debug-mode", po::bool_switch(&use_debug_runner), "start execution in debugger (using gdb rsp interface)")
		("debug-port", po::value<unsigned int>(&debug_port), "select port number to connect with GDB")
		("trace-mode", po::bool_switch(&trace_mode), "enable instruction tracing")
		("tlm-global-quantum", po::value<unsigned int>(&tlm_global_quantum), "set global tlm quantum (in NS)")
		("use-instr-dmi", po::bool_switch(&use_instr_dmi), "use dmi to fetch instructions")
		("use-data-dmi", po::bool_switch(&use_data_dmi), "use dmi to execute load/store operations")
		("use-dmi", po::bool_switch(), "use instr and data dmi")
		("input-file", po::value<std::string>(&input_program)->required(), "input file to use for execution");
	// clang-format on

	pos.add("input-file", 1);
}

void Options::parse(int argc, char **argv) {
	try {
		auto parser = po::command_line_parser(argc, argv);
		parser.options(*this).positional(pos);

		po::store(parser.run(), vm);

		if (vm.count("help")) {
			std::cout << *this << std::endl;
			exit(0);
		}

		po::notify(vm);
		if (vm["use-dmi"].as<bool>()) {
			use_data_dmi = true;
			use_instr_dmi = true;
		}
	} catch (po::error &e) {
		std::cerr
			<< "Error parsing command line options: "
			<< e.what()
			<< std::endl;
		exit(1);
	}
}
