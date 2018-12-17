file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_compile_definitions(COLOR_THEME_DARK)

add_library(rv32
		gdb_stub.cpp
		instr.cpp
		iss.cpp
		syscall.cpp
        ${HEADERS})

target_include_directories(rv32 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
