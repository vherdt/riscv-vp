file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(rv32
		gdb_stub.cpp
		instr.cpp
		iss.cpp
		syscall.cpp
        ${HEADERS})

#target_compile_definitions(rv32 PRIVATE COLOR_THEME_LIGHT)

target_include_directories(rv32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
