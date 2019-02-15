.globl _start
_start:

ld a1, doubleword
addi a0, a1, 1
addi a2, a1, 2
lwu a3, doubleword
lb a4, doubleword
lh a5, doubleword
lw a6, doubleword

# call exit (SYS_EXIT=93) with exit code 0 (argument in a0)
li a7,93
li a0,0

ecall



.section .rodata
doubleword:
	    .dword 1234394967295



