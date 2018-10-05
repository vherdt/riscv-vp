#include <cstdlib>
#include <ctime>

#include "iss.h"
#include "memory.h"
#include "mram.h"
#include "flash.h"
#include "elf_loader.h"
#include "plic.h"
#include "clint.h"
#include "terminal.h"
#include "sensor.h"
#include "sensor2.h"
#include "basic_timer.h"
#include "ethernet.h"
#include "display.hpp"
#include "dma.h"
#include "gdb_stub.h"

#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/io/ios_state.hpp>


struct Options {
    typedef unsigned int addr_t;

    Options &check_and_post_process() {
        mem_end_addr = mem_start_addr + mem_size - 1;
    	assert((mem_end_addr < clint_start_addr || mem_start_addr > display_end_addr) && "RAM too big, would overlap memory");
        mram_end_addr = mram_start_addr + mram_size - 1;
        assert(mram_end_addr < dma_start_addr && "MRAM too big, would overlap memory");
        return *this;
    }

    std::string input_program;
    std::string mram_image;
    std::string flash_device;
    std::string network_device;
    std::string test_signature;

    addr_t mem_size           = 1024*1024*32;  // 32 MB ram, to place it before the CLINT and run the base examples (assume memory start at zero) without modifications
    addr_t mem_start_addr     = 0x00000000;
    addr_t mem_end_addr       = mem_start_addr + mem_size - 1;
    addr_t clint_start_addr   = 0x02000000;
    addr_t clint_end_addr     = 0x0200ffff;
    addr_t term_start_addr    = 0x20000000;
    addr_t term_end_addr      = term_start_addr + 16;
    addr_t ethernet_start_addr= 0x30000000;
    addr_t ethernet_end_addr  = ethernet_start_addr + 1500;
    addr_t plic_start_addr    = 0x40000000;
    addr_t plic_end_addr      = 0x40001000;
    addr_t sensor_start_addr  = 0x50000000;
    addr_t sensor_end_addr    = 0x50001000;
    addr_t sensor2_start_addr = 0x50002000;
    addr_t sensor2_end_addr   = 0x50004000;
    addr_t mram_start_addr    = 0x60000000;
    addr_t mram_size		  = 0x10000000;
    addr_t mram_end_addr      = mram_start_addr + mram_size - 1;
    addr_t dma_start_addr     = 0x70000000;
    addr_t dma_end_addr       = 0x70001000;
    addr_t flash_start_addr   = 0x71000000;
    addr_t flash_end_addr     = flash_start_addr + Flashcontroller::ADDR_SPACE;	//Usually 528 Byte
    addr_t display_start_addr = 0x72000000;
    addr_t display_end_addr   = display_start_addr + Display::addressRange;


    bool use_debug_runner = false;
    bool use_instr_dmi = false;
    bool use_data_dmi = false;

    unsigned int tlm_global_quantum = 10;

    void show() {
        std::cout << "options {" << std::endl;
        std::cout << "  use instr dmi = " << use_instr_dmi << std::endl;
        std::cout << "  use data dmi = " << use_data_dmi << std::endl;
        std::cout << "  tlm global quantum = " << tlm_global_quantum << std::endl;
        std::cout << "}" << std::endl;
    }
};


Options parse_command_line_arguments(int argc, char **argv) {
    // Note: first check for *help* argument then run *notify*, see: https://stackoverflow.com/questions/5395503/required-and-optional-arguments-using-boost-library-program-options
    try {
        Options opt;

        namespace po = boost::program_options;

        po::options_description desc("Options");

        desc.add_options()
                ("help", "produce help message")
                ("memory-start", po::value<unsigned int>(&opt.mem_start_addr), "set memory start address")
                ("debug-mode", po::bool_switch(&opt.use_debug_runner), "start execution in debugger (using gdb rsp interface)")
                ("tlm-global-quantum", po::value<unsigned int>(&opt.tlm_global_quantum), "set global tlm quantum (in NS)")
                ("use-instr-dmi", po::bool_switch(&opt.use_instr_dmi), "use dmi to fetch instructions")
                ("use-data-dmi", po::bool_switch(&opt.use_data_dmi), "use dmi to execute load/store operations")
                ("use-dmi", po::bool_switch(), "use instr and data dmi")
                ("input-file", po::value<std::string>(&opt.input_program)->required(), "input file to use for execution")
                ("mram-image", po::value<std::string>(&opt.mram_image)->default_value(""), "MRAM image file for persistency")
                ("mram-image-size", po::value<unsigned int>(&opt.mram_size), "MRAM image size")
                ("flash-device", po::value<std::string>(&opt.flash_device)->default_value(""), "blockdevice for flash emulation")
                ("network-device", po::value<std::string>(&opt.network_device)->default_value(""), "name of the tap network adapter, e.g. /dev/tap6")
                ("signature", po::value<std::string>(&opt.test_signature)->default_value(""), "output filename for the test execution signature")
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

    std::srand(std::time(nullptr)); // use current time as seed for random generator

    tlm::tlm_global_quantum::instance().set(sc_core::sc_time(opt.tlm_global_quantum, sc_core::SC_NS));

    ISS core;
    SimpleMemory mem("SimpleMemory", opt.mem_size);
    SimpleTerminal term("SimpleTerminal");
    ELFLoader loader(opt.input_program.c_str());
    SimpleBus<2,11> bus("SimpleBus");
    CombinedMemoryInterface iss_mem_if("MemoryInterface", core.quantum_keeper);
    SyscallHandler sys;
    PLIC plic("PLIC");
    CLINT clint("CLINT");
    SimpleSensor sensor("SimpleSensor", 2);
    SimpleSensor2 sensor2("SimpleSensor2", 5);
    BasicTimer timer("BasicTimer", 3);
    SimpleMRAM mram("SimpleMRAM", opt.mram_image, opt.mram_size);
    SimpleDMA dma("SimpleDMA", 4);
    Flashcontroller flashController("Flashcontroller", opt.flash_device);
    EthernetDevice ethernet("EthernetDevice", 7, mem.data, opt.network_device);
    Display display("Display");


    direct_memory_interface dmi({mem.data, opt.mem_start_addr, mem.size});
    InstrMemoryProxy instr_mem(dmi, core.quantum_keeper);
    DataMemoryProxy data_mem(dmi, &iss_mem_if, core.quantum_keeper);

    instr_memory_interface *instr_mem_if = &iss_mem_if;
    data_memory_interface *data_mem_if = &iss_mem_if;
    if (opt.use_instr_dmi)
        instr_mem_if = &instr_mem;
    if (opt.use_data_dmi)
        data_mem_if = &data_mem;


    bus.ports[0] = new PortMapping(opt.mem_start_addr, opt.mem_end_addr);
    bus.ports[1] = new PortMapping(opt.term_start_addr, opt.term_end_addr);
    bus.ports[2] = new PortMapping(opt.plic_start_addr, opt.plic_end_addr);
    bus.ports[3] = new PortMapping(opt.sensor_start_addr, opt.sensor_end_addr);
    bus.ports[4] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr);
    bus.ports[5] = new PortMapping(opt.dma_start_addr, opt.dma_end_addr);
    bus.ports[6] = new PortMapping(opt.sensor2_start_addr, opt.sensor2_end_addr);
    bus.ports[7] = new PortMapping(opt.mram_start_addr, opt.mram_end_addr);
    bus.ports[8] = new PortMapping(opt.flash_start_addr, opt.flash_end_addr);
    bus.ports[9] = new PortMapping(opt.ethernet_start_addr, opt.ethernet_end_addr);
    bus.ports[10]= new PortMapping(opt.display_start_addr, opt.display_end_addr);

    loader.load_executable_image(mem.data, mem.size, opt.mem_start_addr);
    core.init(instr_mem_if, data_mem_if, &clint, &sys, loader.get_entrypoint(), opt.mem_end_addr-4); // -4 to not overlap with the next region
    sys.init(mem.data, opt.mem_start_addr, loader.get_heap_addr());

    // connect TLM sockets
    iss_mem_if.isock.bind(bus.tsocks[0]);
    dma.isock.bind(bus.tsocks[1]);
    bus.isocks[0].bind(mem.tsock);
    bus.isocks[1].bind(term.tsock);
    bus.isocks[2].bind(plic.tsock);
    bus.isocks[3].bind(sensor.tsock);
    bus.isocks[4].bind(clint.tsock);
    bus.isocks[5].bind(dma.tsock);
    bus.isocks[6].bind(sensor2.tsock);
    bus.isocks[7].bind(mram.tsock);
    bus.isocks[8].bind(flashController.tsock);
    bus.isocks[9].bind(ethernet.tsock);
    bus.isocks[10].bind(display.tsock);

    // connect interrupt signals/communication
    plic.target_hart = &core;
    clint.target_hart = &core;
    sensor.plic = &plic;
    dma.plic = &plic;
    timer.plic = &plic;
    sensor2.plic = &plic;
    ethernet.plic = &plic;


    if (opt.use_debug_runner) {
        debug_memory_mapping dmm({mem.data, opt.mem_start_addr, mem.size});
        new DebugCoreRunner(core, dmm);
    } else {
        new DirectCoreRunner(core);
    }

    sc_core::sc_start();

    core.show();

    if (opt.test_signature != "") {
        auto begin_sig = loader.get_begin_signature_address();
        auto end_sig = loader.get_end_signature_address();

        {
            boost::io::ios_flags_saver ifs(cout);
            std::cout << std::hex;
            std::cout << "begin_signature: " << begin_sig << std::endl;
            std::cout << "end_signature: " << end_sig << std::endl;
            std::cout << "signature output file: " << opt.test_signature << std::endl;
        }

        assert (end_sig >= begin_sig);
        assert (begin_sig >= opt.mem_start_addr);

        auto begin = begin_sig - opt.mem_start_addr;
        auto end = end_sig - opt.mem_start_addr;

        ofstream sigfile(opt.test_signature, ios::out);

        auto n = begin;
        while (n < end) {
            sigfile << std::hex << std::setw(2) << std::setfill('0') << (unsigned)mem.data[n];
            ++n;
        }
    }

	return 0;
}
