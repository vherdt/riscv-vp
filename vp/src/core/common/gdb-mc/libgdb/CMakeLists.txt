add_library(gdb
	mpc/mpc.c
	parser1.c
	parser2.c
	util.c)

set_source_files_properties(mpc/mpc.c
	PROPERTIES COMPILE_FLAGS "-w -ansi")
target_include_directories(gdb PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/mpc)
target_include_directories(gdb PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include)
