all : main.o
	riscv32-unknown-elf-ld main.o -o main
	
main.o : main.S
	riscv32-unknown-elf-as main.S -o main.o -march=rv32i -mabi=ilp32
	
dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
64main.o : main.S
	riscv64-unknown-elf-as main.S -o 64main.o -march=rv64i -mabi=lp64

rv64ima: 64main.o
	riscv64-unknown-elf-ld 64main.o -o main64

clean:
	rm -f main main.o main64 64main.o
