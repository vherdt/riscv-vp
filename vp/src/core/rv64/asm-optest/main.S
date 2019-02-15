.globl _start
_start:

li a1,0x7FFFFFFE #2147483646
li a2,1
li a3,0
li a4,0
li a5,0

li a6,1

#add a3,a1,a1
addw a3,a1,a2
addw a4,a3,a2
addw a5,a4,a2
addw a6,a1,a1

#subw  a3,a6,a6
#subw  a4,a3,a6
#subw  a5,a4,a6
#subw  a6,a2,a1


#addiw a4,a1,1
#addiw a5,a1,11
#addiw a6,a1,0x7FF # +2047 = 2045
#li a1,0xFFFF7FFFFFFE
#addiw a3,a1,0  //write lower bits

#subiw a3,a1,0
#subiw a4,a1,1
#subiw a5,a1,11

#slliw a2,a1,29
#slliw a3,a1,1
#slliw a4,a3,14
#srliw a5,a1,30
#slliw a6,a1,30
#sraiw a1,a2,4

#slli a2,a1,12
#li a3,0xBFFFFFFFFFFFFFFF
#sraiw a4,a3,1
#srai a5,a3,1




# call exit (SYS_EXIT=93) with exit code 0 (argument in a0)
li a7,93
li a0,0
ecall
