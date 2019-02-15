all : sum.o
	riscv32-unknown-elf-ld sum.o -o main
	
sum.o : sum.S
	riscv32-unknown-elf-as sum.S -o sum.o -march=rv32i -mabi=ilp32
	
dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
64sum.o : sum.S
	riscv64-unknown-elf-as sum.S -o 64sum.o -march=rv64i -mabi=lp64

rv64ima: 64sum.o
	riscv64-unknown-elf-ld 64sum.o -o main

clean:
	rm -f main sum.o
