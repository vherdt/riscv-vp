.globl _start
.globl register_handler
.globl main

_start:
call register_handler
jal main

# call exit (SYS_EXIT=93) with exit code 0 (argument in a0)
li a7,93
li a0,0
ecall
