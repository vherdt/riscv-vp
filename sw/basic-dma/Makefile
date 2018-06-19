all : main.c irq.c irq.h bootstrap.S
	riscv32-unknown-elf-gcc main.c irq.c bootstrap.S -o main -march=rv32g -mabi=ilp32d -nostartfiles -Wl,--no-relax
	
dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
dump-comment: all
	objdump -s --section .comment main
	
clean:
	rm -f main
