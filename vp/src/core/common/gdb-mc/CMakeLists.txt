add_library(gdb-mc
	gdb_server.cpp)

target_link_libraries(gdb-mc core-common)
