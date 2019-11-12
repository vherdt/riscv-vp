add_library(gdb-mc
	protocol/mpc/mpc.c
	protocol/parser.c
	protocol/parser2.c
	protocol/util.c
	gdb_server.cpp
	handler.cpp)

set_source_files_properties(protocol/mpc/mpc.c
	PROPERTIES COMPILE_FLAGS "-w -ansi")
target_include_directories(gdb-mc PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/protocol/mpc)

target_link_libraries(gdb-mc core-common)
