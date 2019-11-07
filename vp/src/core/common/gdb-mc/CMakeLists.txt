add_library(gdb-mc
	protocol/mpc/mpc.c
	protocol/parser.c
	gdb_server.cpp)

set_source_files_properties(protocol/mpc/mpc.c
	PROPERTIES COMPILE_FLAGS "-w -ansi")
target_include_directories(gdb-mc PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/protocol/mpc)

target_link_libraries(gdb-mc core-common)
