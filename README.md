# RISC-V based Virtual Prototype (VP)

### Key features of our VP:

 - RV32IMAC core + machine mode CSRs
 - Implemented in SystemC TLM-2.0
 - SW debug capabilities (GDB RSP interface) with Eclipse
 - FreeRTOS support
 - Generic and configurable bus
 - CLINT and PLIC-based interrupt controller + additional peripherals
 - Instruction-based timing model + annotated TLM 2.0 transaction delays
 - **New:** Compressed instructions (C)
 - **New:** Peripherals, e.g. display, flash controller, preliminary ethernet
 - **New:** Example configuration for the SiFive HiFive1 board available

For related information, e.g. verification, please visit http://www.systemc-verification.org/ or contact <riscv@systemc-verification.org>. 
We accept pull requests and in general contributions are very welcome. 

In the following we provide build instructions and how to compile and run software on the VP.


#### 1) Build the RISC-V GNU Toolchain:

(Cross-)Compiling the software examples, in order to run them on the VP, requires the RISC-V GNU toolchain to be available in PATH. Several standard packages are required to build the toolchain. On Ubuntu the required packages can be installed as follows:

```bash
sudo apt-get install autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo gperf libtool patchutils bc zlib1g-dev libexpat-dev
```

On Fedora, following actions are required:
```bash
sudo dnf install autoconf automake curl libmpc-devel mpfr-devel gmp-devel gawk bison flex texinfo gperf libtool patchutils bc zlib-devel expat-devel cmake boost-devel
sudo dnf groupinstall "C Development Tools and Libraries"
#optional debuginfo
sudo dnf debuginfo-install boost-iostreams boost-program-options boost-regex bzip2-libs glibc libgcc libicu libstdc++ zlib
```

For more information on prerequisites for the RISC-V GNU toolchain visit https://github.com/riscv/riscv-gnu-toolchain. With the packages installed, the toolchain can be build as follows:

```bash
git clone https://github.com/riscv/riscv-gnu-toolchain.git
cd riscv-gnu-toolchain
git submodule update --init --recursive

./configure --prefix=$(pwd)/../riscv-gnu-toolchain-dist-rv32g-ilp32d --with-arch=rv32g --with-abi=ilp32d

make
```


#### 2) Build this RISC-V Virtual Prototype:

i) in *vp/dependencies* folder (will download and compile SystemC):

```bash
./build_systemc_232.sh
```


ii) in *vp* folder (requires the *boost* C++ library):
 
```bash
mkdir build
cd build
cmake ..
make
```


#### 3) Compile and run some Software:

In *sw*:

```bash
cd simple-sensor                                   # can be replaced with different example
make                                               # (requires RISC-V GNU toolchain in PATH)
../../vp/build/src/platform/basic/riscv-vp main    # shows final simulation time as well as register and pc contents
```

Add the *riscv-vp* executable to PATH to simplify execution of SW examples.


#### 4) Optional Makefile:

The toplevel Makefile can alternatively be used to build the VP including its dependencies (i.e. step 2 in this README), from the toplevel folder call:

```bash
make
```

This will also copy the VP binaries into the *vp/build/bin* folder.
