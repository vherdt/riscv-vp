all : main.c sum.c bootstrap.S
	riscv32-unknown-elf-gcc main.c sum.c bootstrap.S -o main -march=rv32i -mabi=ilp32 -nostartfiles
	
dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
dump-comment: all
	objdump -s --section .comment main

rv64ima: main.c sum.c bootstrap.S
	riscv64-unknown-elf-gcc main.c sum.c bootstrap.S -o main -march=rv64i -mabi=lp64 -nostartfiles

dump-64elf: rv64ima
	riscv64-unknown-elf-readelf -a main
	
dump-64code: rv64ima
	riscv64-unknown-elf-objdump -D main

dump-64comment: rv64ima
	objdump -s --section .comment main
clean:
	rm -f main
