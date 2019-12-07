add_library(gdb
	mpc/mpc.c
	parser1.c
	parser2.c
	response.c
	util.c)

target_compile_options(gdb PRIVATE
	-Wpedantic -Wall -Wextra -Wconversion -Wmissing-prototypes
	-Wpointer-arith -Wstrict-prototypes -Wshadow -Wcast-align)

set_source_files_properties(mpc/mpc.c
	PROPERTIES COMPILE_FLAGS "-w -ansi")
target_include_directories(gdb PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/mpc)
target_include_directories(gdb PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include)
