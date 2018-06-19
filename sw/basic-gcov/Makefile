all : main.c
	riscv32-unknown-elf-gcc main.c --coverage -o main -march=rv32g -mabi=ilp32d
	
main.gcda : all
	../../vp/build/lib/riscv-vp main
	
# use -b option to display branch coverage too
dump-coverage: main.gcda
	riscv32-unknown-elf-gcov main.c
	
dump-elf: all
	riscv32-unknown-elf-readelf -a main
	
dump-code: all
	riscv32-unknown-elf-objdump -D main
	
clean:
	rm -f main main.c.gcov main.gcda main.gcno
