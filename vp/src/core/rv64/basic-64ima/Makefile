all : main.c bootstrap.S
	riscv64-unknown-elf-gcc main.c bootstrap.S -o main -nostartfiles -march=rv64ima -mabi=lp64 
	
dump-elf: all
	riscv64-unknown-elf-readelf -a main
	
dump-code: all
	riscv64-unknown-elf-objdump -D main
	
dump-comment: all
	objdump -s --section .comment main
	
clean:
	rm -f main
