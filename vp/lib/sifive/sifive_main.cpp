#include <cstdlib>
#include <ctime>

#include "iss.h"
#include "memory.h"
#include "elf_loader.h"
#include "plic.h"
#include "clint.h"
#include "aon.h"
#include "prci.h"
#include "spi.h"
#include "uart.h"
#include "gpio.h"

#include <iostream>
#include <iomanip>
#include <boost/program_options.hpp>
#include <boost/io/ios_state.hpp>


struct Options {
    typedef unsigned int addr_t;

    Options &check_and_post_process() {
        return *this;
    }

    std::string input_program;

    addr_t dram_size          = 1024*16;  // 16 KB dram
    addr_t dram_start_addr    = 0x80000000;
    addr_t dram_end_addr      = dram_start_addr + dram_size - 1;
    addr_t flash_size         = 1024*1024*512;  // 512 MB flash
    addr_t flash_start_addr   = 0x20000000;
    addr_t flash_end_addr     = flash_start_addr + flash_size - 1;
    addr_t clint_start_addr   = 0x02000000;
    addr_t clint_end_addr     = 0x0200FFFF;
    addr_t plic_start_addr    = 0x0C000000;
    addr_t plic_end_addr      = 0x0FFFFFFF;
    addr_t aon_start_addr     = 0x10000000;
    addr_t aon_end_addr       = 0x10007FFF;
    addr_t prci_start_addr    = 0x10008000;
    addr_t prci_end_addr      = 0x1000FFFF;
    addr_t uart0_start_addr   = 0x10013000;
    addr_t uart0_end_addr     = 0x10013FFF;
    addr_t spi0_start_addr    = 0x10014000;
    addr_t spi0_end_addr      = 0x10014FFF;
    addr_t gpio0_start_addr   = 0x10012000;
    addr_t gpio0_end_addr     = 0x10012FFF;


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
                ("tlm-global-quantum", po::value<unsigned int>(&opt.tlm_global_quantum), "set global tlm quantum (in NS)")
                ("use-instr-dmi", po::bool_switch(&opt.use_instr_dmi), "use dmi to fetch instructions")
                ("use-data-dmi", po::bool_switch(&opt.use_data_dmi), "use dmi to execute load/store operations")
                ("use-dmi", po::bool_switch(), "use instr and data dmi")
                ("input-file", po::value<std::string>(&opt.input_program)->required(), "input file to use for execution")
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
    SimpleMemory dram("DRAM", opt.dram_size);
    SimpleMemory flash("Flash", opt.flash_size);
    ELFLoader loader(opt.input_program.c_str());
    SimpleBus<1,9> bus("SimpleBus");
    CombinedMemoryInterface iss_mem_if("MemoryInterface", core.quantum_keeper);
    SyscallHandler sys;
    PLIC plic("PLIC");
    CLINT clint("CLINT");
    AON aon("AON");
    PRCI prci("PRCI");
    SPI spi0("SPI0");
    UART uart0("UART0");
    GPIO gpio0("GPIO0");


    direct_memory_interface dram_dmi({dram.data, opt.dram_start_addr, dram.size});
    direct_memory_interface flash_dmi({flash.data, opt.flash_start_addr, flash.size});
    InstrMemoryProxy instr_mem(flash_dmi, core.quantum_keeper);
    DataMemoryProxy data_mem(dram_dmi, &iss_mem_if, core.quantum_keeper);

    instr_memory_interface *instr_mem_if = &iss_mem_if;
    data_memory_interface *data_mem_if = &iss_mem_if;
    if (opt.use_instr_dmi)
        instr_mem_if = &instr_mem;
    if (opt.use_data_dmi)
        data_mem_if = &data_mem;


    bus.ports[0] = new PortMapping(opt.flash_start_addr, opt.flash_end_addr);
    bus.ports[1] = new PortMapping(opt.dram_start_addr, opt.dram_end_addr);
    bus.ports[2] = new PortMapping(opt.plic_start_addr, opt.plic_end_addr);
    bus.ports[3] = new PortMapping(opt.clint_start_addr, opt.clint_end_addr);
    bus.ports[4] = new PortMapping(opt.aon_start_addr, opt.aon_end_addr);
    bus.ports[5] = new PortMapping(opt.prci_start_addr, opt.prci_end_addr);
    bus.ports[6] = new PortMapping(opt.spi0_start_addr, opt.spi0_end_addr);
    bus.ports[7] = new PortMapping(opt.uart0_start_addr, opt.uart0_end_addr);
    bus.ports[8] = new PortMapping(opt.gpio0_start_addr, opt.gpio0_end_addr);


    for (auto p : loader.get_load_sections()) {
        if (p->p_vaddr >= opt.dram_start_addr && p->p_vaddr < opt.dram_end_addr)
            memcpy(dram.data+p->p_vaddr-opt.dram_start_addr, loader.elf.data()+p->p_offset, p->p_filesz);
        else if (p->p_vaddr >= opt.flash_start_addr && p->p_vaddr < opt.flash_end_addr)
            memcpy(flash.data+p->p_vaddr-opt.flash_start_addr, loader.elf.data()+p->p_offset, p->p_filesz);
        else
            assert (false);
    }

    core.init(instr_mem_if, data_mem_if, &clint, &sys, loader.get_entrypoint(), opt.dram_end_addr-4); // -4 to not overlap with the next region
    sys.init(dram.data, opt.dram_start_addr, loader.get_heap_addr());

    // connect TLM sockets
    iss_mem_if.isock.bind(bus.tsocks[0]);
    bus.isocks[0].bind(flash.tsock);
    bus.isocks[1].bind(dram.tsock);
    bus.isocks[2].bind(plic.tsock);
    bus.isocks[3].bind(clint.tsock);
    bus.isocks[4].bind(aon.tsock);
    bus.isocks[5].bind(prci.tsock);
    bus.isocks[6].bind(spi0.tsock);
    bus.isocks[7].bind(uart0.tsock);
    bus.isocks[8].bind(gpio0.tsock);

    // connect interrupt signals/communication
    plic.target_hart = &core;
    clint.target_hart = &core;


    new DirectCoreRunner(core);

    sc_core::sc_start();

    core.show();


    return 0;
}
