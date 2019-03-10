file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h gdb_stub.tpp)

add_library(core-common
		instr.cpp
		gdb_stub.cpp
		${HEADERS})

target_include_directories(core-common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
