.globl _start
_start:

li a0,50
li a1,100

li a2,0
loop:
bgt a0,a1,end
add a2,a2,a0
addi a0,a0,1
j loop
end:

hlt:
j hlt
