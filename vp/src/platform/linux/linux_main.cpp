#include <cstdlib>
#include <ctime>

#include "core/common/clint.h"
#include "elf_loader.h"
#include "fu540_plic.h"
#include "gdb_stub.h"
#include "iss.h"
#include "mem.h"
#include "memory.h"
#include "mmu.h"
#include "platform/common/slip.h"
#include "platform/common/uart.h"
#include "prci.h"
#include "syscall.h"
#include "debug.h"
#include "util/options.h"

#include "gdb-mc/gdb_server.h"
#include "gdb-mc/gdb_runner.h"

#include <boost/io/ios_state.hpp>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>

#include <termios.h>
#include <unistd.h>

enum {
	NUM_CORES = 5,
};

using namespace rv64;

struct Options {
	typedef unsigned int addr_t;

	Options &check_and_post_process() {
		entry_point.finalize(parse_ulong_option);

		mem_end_addr = mem_start_addr + mem_size - 1;
		return *this;
	}

	std::string input_program;

	addr_t mem_size = 1024u * 1024u * 2048u;  // 2048 MB ram
	addr_t mem_start_addr = 0x80000000;
	addr_t mem_end_addr = mem_start_addr + mem_size - 1;
	addr_t clint_start_addr = 0x02000000;
	addr_t clint_end_addr = 0x0200ffff;
	addr_t sys_start_addr = 0x02010000;
	addr_t sys_end_addr = 0x020103ff;
	addr_t dtb_rom_start_addr = 0x00001000;
	addr_t dtb_rom_size = 0x2000;
	addr_t dtb_rom_end_addr = dtb_rom_start_addr + dtb_rom_size - 1;
	addr_t uart0_start_addr = 0x10010000;
	addr_t uart0_end_addr = 0x10010fff;
	addr_t uart1_start_addr = 0x10011000;
	addr_t uart1_end_addr = 0x10011fff;
	addr_t plic_start_addr = 0x0C000000;
	addr_t plic_end_addr = 0x10000000;
	addr_t prci_start_addr = 0x10000000;
	addr_t prci_end_addr = 0x1000FFFF;

	bool use_debug_runner = false;
	bool use_instr_dmi = false;
	bool use_data_dmi = false;
	bool trace_mode = false;
	bool intercept_syscalls = false;
	unsigned int debug_port = 5005;

	unsigned int tlm_global_quantum = 10;

	OptionValue<unsigned long> entry_point;
	std::string dtb_file;
	std::string tun_device = "tun0";
};

class Core {
   public:
	ISS iss;
	MMU mmu;
	CombinedMemoryInterface memif;
	InstrMemoryProxy imemif;

	Core(unsigned int id, MemoryDMI dmi)
	    : iss(id), mmu(iss), memif(("MemoryInterface" + std::to_string(id)).c_str(), iss, mmu), imemif(dmi, iss) {
		return;
	}

	void init(bool use_data_dmi, bool use_instr_dmi, clint_if *clint, uint64_t entry, uint64_t addr) {
		if (use_data_dmi)
			memif.dmi_ranges.emplace_back(imemif.dmi);

		iss.init(get_instr_memory_if(use_instr_dmi), &memif, clint, entry, addr);
	}

   private:
	instr_memory_if *get_instr_memory_if(bool use_instr_dmi) {
		if (use_instr_dmi)
			return &imemif;
		else
			return &memif;
	}
};

Options parse_command_line_arguments(int argc, char **argv) {
	// Note: first check for *help* argument then run *notify*, see:
	// https://stackoverflow.com/questions/5395503/required-and-optional-arguments-using-boost-library-program-options
	try {
		Options opt;

		namespace po = boost::program_options;

		po::options_description desc("Options");

        // clang-format off
		desc.add_options()
		("help", "produce help message")
		("memory-start", po::value<unsigned int>(&opt.mem_start_addr),"set memory start address")
		("memory-size", po::value<unsigned int>(&opt.mem_size), "set memory size")
		("intercept-syscalls", po::bool_switch(&opt.intercept_syscalls),"directly intercept and handle syscalls in the ISS")
		("debug-mode", po::bool_switch(&opt.use_debug_runner),"start execution in debugger (using gdb rsp interface)")
		("debug-port", po::value<unsigned int>(&opt.debug_port), "select port number to connect with GDB")
		("entry-point", po::value<std::string>(&opt.entry_point.option),"set entry point address (ISS program counter)")
		("trace-mode", po::bool_switch(&opt.trace_mode),"enable instruction tracing")
		("tlm-global-quantum", po::value<unsigned int>(&opt.tlm_global_quantum), "set global tlm quantum (in NS)")
		("use-instr-dmi", po::bool_switch(&opt.use_instr_dmi), "use dmi to fetch instructions")
		("use-data-dmi", po::bool_switch(&opt.use_data_dmi), "use dmi to execute load/store operations")
		("use-dmi", po::bool_switch(), "use instr and data dmi")
		("input-file", po::value<std::string>(&opt.input_program)->required(), "input file to use for execution")
		("dtb-file", po::value<std::string>(&opt.dtb_file)->required(), "dtb file for boot loading")
		("tun-device", po::value<std::string>(&opt.tun_device), "tun device used by SLIP");
        // clang-format on

		po::positional_options_description pos;
		pos.add("input-file", 1);

		po::variables_map vm;
		po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);

		if (vm.count("help")) {
			std::cout << desc << std::endl;
			exit(0);
		}

		po::notify(vm);

		if (vm["use-dmi"].as<bool>()) {
			opt.use_data_dmi = true;
			opt.use_instr_dmi = true;
		}

		return opt.check_and_post_process();
	} catch (boost::program_options::error &e) {
		std::cerr << "Error parsing command line options: " << e.what() << std::endl;
		exit(-1);
	}
}

int sc_main(int argc, char **argv) {
	Options opt = parse_command_line_arguments(argc, argv);

	std::srand(std::time(nullptr));  // use current time as seed for random generator

	tlm::tlm_global_quantum::instance().set(sc_core::sc_time(opt.tlm_global_quantum, sc_core::SC_NS));

	SimpleMemory mem("SimpleMemory", opt.mem_size);
	SimpleMemory dtb_rom("DBT_ROM", opt.dtb_rom_size);
	ELFLoader loader(opt.input_program.c_str());
	SimpleBus<NUM_CORES + 1, 8> bus("SimpleBus");
	SyscallHandler sys("SyscallHandler");
	FU540_PLIC plic("PLIC", NUM_CORES);
	CLINT<NUM_CORES> clint("CLINT");
	PRCI prci("PRCI");
	UART uart0("UART0", 3);
	SLIP slip("SLIP", 4, opt.tun_device);
	DebugMemoryInterface dbg_if("DebugMemoryInterface");
	MemoryDMI dmi = MemoryDMI::create_start_size_mapping(mem.data, opt.mem_start_addr, mem.size);

	Core *cores[NUM_CORES];
	for (unsigned i = 0; i < NUM_CORES; i++) {
		cores[i] = new Core(i, dmi);
	}

	std::shared_ptr<BusLock> bus_lock = std::make_shared<BusLock>();
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->memif.bus_lock = bus_lock;
		cores[i]->mmu.mem = &cores[i]->memif;
	}

	uint64_t entry_point = loader.get_entrypoint();
	if (opt.entry_point.available)
		entry_point = opt.entry_point.value;

	loader.load_executable_image(mem.data, mem.size, opt.mem_start_addr);
	sys.init(mem.data, opt.mem_start_addr, loader.get_heap_addr());
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->init(opt.use_data_dmi, opt.use_instr_dmi, &clint, entry_point, rv64_align_address(opt.mem_end_addr));

		sys.register_core(&cores[i]->iss);
		if (opt.intercept_syscalls)
			cores[i]->iss.sys = &sys;
	}

	// setup port mapping
	bus.ports[0] = new PortMapping(opt.mem_start_addr, opt.mem_end_addr);
	bus.ports[1] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr);
	bus.ports[2] = new PortMapping(opt.sys_start_addr, opt.sys_end_addr);
	bus.ports[3] = new PortMapping(opt.dtb_rom_start_addr, opt.dtb_rom_end_addr);
	bus.ports[4] = new PortMapping(opt.uart0_start_addr, opt.uart0_end_addr);
	bus.ports[5] = new PortMapping(opt.uart1_start_addr, opt.uart1_end_addr);
	bus.ports[6] = new PortMapping(opt.plic_start_addr, opt.plic_end_addr);
	bus.ports[7] = new PortMapping(opt.prci_start_addr, opt.prci_end_addr);

	// connect TLM sockets
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->memif.isock.bind(bus.tsocks[i]);
	}
	dbg_if.isock.bind(bus.tsocks[NUM_CORES]);
	bus.isocks[0].bind(mem.tsock);
	bus.isocks[1].bind(clint.tsock);
	bus.isocks[2].bind(sys.tsock);
	bus.isocks[3].bind(dtb_rom.tsock);
	bus.isocks[4].bind(uart0.tsock);
	bus.isocks[5].bind(slip.tsock);
	bus.isocks[6].bind(plic.tsock);
	bus.isocks[7].bind(prci.tsock);

	// connect interrupt signals/communication
	for (size_t i = 0; i < NUM_CORES; i++) {
		plic.target_harts[i] = &cores[i]->iss;
		clint.target_harts[i] = &cores[i]->iss;
	}
	uart0.plic = &plic;
	slip.plic = &plic;

	for (size_t i = 0; i < NUM_CORES; i++) {
		// switch for printing instructions
		cores[i]->iss.trace = opt.trace_mode;

		// ignore WFI instructions (handle them as a NOP, which is ok according to the RISC-V ISA) to avoid running too
		// fast ahead with simulation time when the CPU is idle
		cores[i]->iss.ignore_wfi = true;

		// emulate RISC-V core boot loader
		cores[i]->iss.regs[RegFile::a0] = cores[i]->iss.get_hart_id();
		cores[i]->iss.regs[RegFile::a1] = opt.dtb_rom_start_addr;
	}

	// OpenSBI boots all harts except hart 0 by default.
	//
	// To prevent this hart from being scheduled when stuck in
	// the OpenSBI `sbi_hart_hang()` function do not ignore WFI on
	// this hart.
	//
	// See: https://github.com/riscv/opensbi/commit/d70f8aab45d1e449b3b9be26e050b20ed76e12e9
	cores[0]->iss.ignore_wfi = false;

	// load DTB (Device Tree Binary) file
	dtb_rom.load_binary_file(opt.dtb_file, 0);

	std::vector<mmu_memory_if*> mmus;
	std::vector<debug_target_if*> dharts;
	if (opt.use_debug_runner) {
		for (size_t i = 0; i < NUM_CORES; i++) {
			dharts.push_back(&cores[i]->iss);
			mmus.push_back(&cores[i]->memif);
		}

		auto server = new GDBServer("GDBServer", dharts, &dbg_if, opt.debug_port, mmus);
		for (size_t i = 0; i < dharts.size(); i++)
			new GDBServerRunner(("GDBRunner" + std::to_string(i)).c_str(), server, dharts[i]);
	} else {
		for (size_t i = 0; i < NUM_CORES; i++) {
			new DirectCoreRunner(cores[i]->iss);
		}
	}

	sc_core::sc_start();
	for (size_t i = 0; i < NUM_CORES; i++) {
		cores[i]->iss.show();
	}

	return 0;
}
