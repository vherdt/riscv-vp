These SW examples demonstrate basic bare-metal applications and access to the C-library. The (primarily) C-library applications require support for syscalls (system calls). Syscalls are handled in one of two ways in the VP:

 1) By default the VP will trigger a trap and thus jump to the trap/interrupt handler. The handler will detect that the reason is a syscall and re-direct the syscall to the syscall handler peripheral through memory mapped I/O. This requires to install a syscall handler. For applications using the C-library, we provide a custom entry-point that installs the trap handler and then jumps to the crt0.S entry code of the C-library (which in turn will call the main function).
 
 2) With the "--intercept-syscalls" command line argument, the VP will directly intercept and handle syscalls, hence it is not necessary to install a trap handler. Thus, no designated startup code (i.e. bootstrap.S) is required to run C-library applications.
 
The provided SW examples demonstrate both approaches for dealing with syscalls.

The VP also supports the FreeRTOS (see https://github.com/agra-uni-bremen/riscv-freertos) and Zephyr (the official RISC-V port, see https://www.zephyrproject.org/ - use the Zephyr hifive board and hifive-vp configuration) operating systems.
