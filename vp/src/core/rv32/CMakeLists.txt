file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(rv32
		gdb_stub.cpp
		instr.cpp
		iss.cpp
		syscall.cpp
        ${HEADERS})

