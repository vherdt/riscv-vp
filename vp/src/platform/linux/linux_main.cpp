#include <cstdlib>
#include <ctime>

#include "core/common/clint.h"
#include "platform/common/uart.h"
#include "platform/common/uart16550.h"
#include "elf_loader.h"
#include "iss.h"
#include "mem.h"
#include "mmu.h"
#include "gdb_stub.h"
#include "memory.h"
#include "plic.h"
#include "syscall.h"
#include "util/options.h"

#include <boost/io/ios_state.hpp>
#include <boost/program_options.hpp>
#include <iomanip>
#include <iostream>

#include <termios.h>
#include <unistd.h>

using namespace rv64;


struct TerminalNoEchoSetting {
    struct termios term;
    struct termios save;
    bool ok;

    TerminalNoEchoSetting() {
        ok = tcgetattr(STDOUT_FILENO, &save) == 0;
        ok = ok && (tcgetattr(STDOUT_FILENO, &term) == 0);
        if (ok) {
            term.c_lflag &= ~((tcflag_t) ECHO);
            if (tcsetattr(STDOUT_FILENO, TCSANOW, &term))
                std::cout << "WARNING: unable to deactivate terminal echo (tcsetattr): " << std::strerror(errno) << std::endl;
        } else {
            std::cout << "WARNING: unable to deactivate terminal echo (tcgetattr): " << std::strerror(errno) << std::endl;
        }
    }

    ~TerminalNoEchoSetting() {
        if (ok)
            tcsetattr(STDOUT_FILENO, TCSANOW, &term);
    }
};


struct Options {
    typedef unsigned int addr_t;

    Options &check_and_post_process() {
        entry_point.finalize(parse_ulong_option);

        mem_end_addr = mem_start_addr + mem_size - 1;
        return *this;
    }

    std::string input_program;

    addr_t mem_size             = 1024u*1024u*2048u;  // 2048 MB ram
    addr_t mem_start_addr       = 0x80000000;
    addr_t mem_end_addr         = mem_start_addr + mem_size - 1;
    addr_t clint_start_addr     = 0x02000000;
    addr_t clint_end_addr       = 0x0200ffff;
    addr_t sys_start_addr       = 0x02010000;
    addr_t sys_end_addr         = 0x020103ff;
    addr_t dtb_rom_start_addr   = 0x00001000;
    addr_t dtb_rom_size         = 0x2000;
    addr_t dtb_rom_end_addr     = dtb_rom_start_addr + dtb_rom_size - 1;
    addr_t uart0_start_addr     = 0x10013000;
    addr_t uart0_end_addr       = 0x10013fff;
    addr_t uart16550_start_addr = 0x10000000;
    addr_t uart16550_end_addr   = 0x100000ff;

    bool use_debug_runner = false;
    bool use_instr_dmi = false;
    bool use_data_dmi = false;
    bool trace_mode = false;
    bool intercept_syscalls = false;
    unsigned int debug_port = 5005;

    unsigned int tlm_global_quantum = 10;

    OptionValue<unsigned long> entry_point;
    std::string dtb_file;
};

Options parse_command_line_arguments(int argc, char **argv) {
    // Note: first check for *help* argument then run *notify*, see:
    // https://stackoverflow.com/questions/5395503/required-and-optional-arguments-using-boost-library-program-options
    try {
        Options opt;

        namespace po = boost::program_options;

        po::options_description desc("Options");

        desc.add_options()
            ("help", "produce help message")
            ("memory-start", po::value<unsigned int>(&opt.mem_start_addr), "set memory start address")
            ("memory-size", po::value<unsigned int>(&opt.mem_size), "set memory size")
            ("intercept-syscalls", po::bool_switch(&opt.intercept_syscalls), "directly intercept and handle syscalls in the ISS")
            ("debug-mode", po::bool_switch(&opt.use_debug_runner), "start execution in debugger (using gdb rsp interface)")
            ("debug-port", po::value<unsigned int>(&opt.debug_port), "select port number to connect with GDB")
            ("entry-point", po::value<std::string>(&opt.entry_point.option), "set entry point address (ISS program counter)")
            ("trace-mode", po::bool_switch(&opt.trace_mode), "enable instruction tracing")
            ("tlm-global-quantum", po::value<unsigned int>(&opt.tlm_global_quantum), "set global tlm quantum (in NS)")
            ("use-instr-dmi", po::bool_switch(&opt.use_instr_dmi), "use dmi to fetch instructions")
            ("use-data-dmi", po::bool_switch(&opt.use_data_dmi), "use dmi to execute load/store operations")
            ("use-dmi", po::bool_switch(), "use instr and data dmi")
            ("input-file", po::value<std::string>(&opt.input_program)->required(), "input file to use for execution")
            ("dtb-file", po::value<std::string>(&opt.dtb_file)->required(), "dtb file for boot loading")
        ;

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

    ISS core(0);
    MMU mmu(core);
    CombinedMemoryInterface core_mem_if("MemoryInterface0", core, mmu);
    SimpleMemory mem("SimpleMemory", opt.mem_size);
    SimpleMemory dtb_rom("DBT_ROM", opt.dtb_rom_size);
    ELFLoader loader(opt.input_program.c_str());
    SimpleBus<2, 6> bus("SimpleBus");
    SyscallHandler sys("SyscallHandler");
    CLINT<1> clint("CLINT");
    UART uart0("UART0");
    UART16550 uart16550("UART16550");
    DebugMemoryInterface dbg_if("DebugMemoryInterface");

    MemoryDMI dmi = MemoryDMI::create_start_size_mapping(mem.data, opt.mem_start_addr, mem.size);
    InstrMemoryProxy instr_mem(dmi, core);

    std::shared_ptr<BusLock> bus_lock = std::make_shared<BusLock>();
    core_mem_if.bus_lock = bus_lock;
    mmu.mem = &core_mem_if;

    instr_memory_if *instr_mem_if = &core_mem_if;
    data_memory_if *data_mem_if = &core_mem_if;
    if (opt.use_instr_dmi)
        instr_mem_if = &instr_mem;
    if (opt.use_data_dmi) {
        core_mem_if.dmi_ranges.emplace_back(dmi);
    }

    uint64_t entry_point = loader.get_entrypoint();
    if (opt.entry_point.available)
        entry_point = opt.entry_point.value;

    loader.load_executable_image(mem.data, mem.size, opt.mem_start_addr);
    core.init(instr_mem_if, data_mem_if, &clint, entry_point, rv64_align_address(opt.mem_end_addr));
    sys.init(mem.data, opt.mem_start_addr, loader.get_heap_addr());
    sys.register_core(&core);

    if (opt.intercept_syscalls)
        core.sys = &sys;

    // setup port mapping
    bus.ports[0] = new PortMapping(opt.mem_start_addr, opt.mem_end_addr);
    bus.ports[1] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr);
    bus.ports[2] = new PortMapping(opt.sys_start_addr, opt.sys_end_addr);
    bus.ports[3] = new PortMapping(opt.dtb_rom_start_addr, opt.dtb_rom_end_addr);
    bus.ports[4] = new PortMapping(opt.uart0_start_addr, opt.uart0_end_addr);
    bus.ports[5] = new PortMapping(opt.uart16550_start_addr, opt.uart16550_end_addr);

    // connect TLM sockets
    core_mem_if.isock.bind(bus.tsocks[0]);
    dbg_if.isock.bind(bus.tsocks[1]);
    bus.isocks[0].bind(mem.tsock);
    bus.isocks[1].bind(clint.tsock);
    bus.isocks[2].bind(sys.tsock);
    bus.isocks[3].bind(dtb_rom.tsock);
    bus.isocks[4].bind(uart0.tsock);
    bus.isocks[5].bind(uart16550.tsock);

    // connect interrupt signals/communication
    clint.target_harts[0] = &core;

    // switch for printing instructions
    core.trace = opt.trace_mode;

    // ignore WFI instructions (handle them as a NOP, which is ok according to the RISC-V ISA) to avoid running too fast ahead with simulation time when the CPU is idle
    core.ignore_wfi = true;

    // emulate RISC-V core boot loader
    core.regs[RegFile::a0] = core.get_hart_id();
    core.regs[RegFile::a1] = opt.dtb_rom_start_addr;

    // load DTB (Device Tree Binary) file
    dtb_rom.load_binary_file(opt.dtb_file, 0);

    if (opt.use_debug_runner) {
        new DebugCoreRunner<ISS, RV64>(core, &dbg_if, opt.debug_port);
    } else {
        new DirectCoreRunner(core);
    }

    // deactivate console echo of host system (the guest system linux has its own echo)
    TerminalNoEchoSetting terminal_no_echo_setting;

    sc_core::sc_start();

    core.show();

    return 0;
}
