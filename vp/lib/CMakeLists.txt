file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(riscv-vp
        main.cpp
        syscall.cpp
        instr.cpp
        gdb_stub.cpp
        ethernet.cpp
        display.cpp
        ${HEADERS})

target_link_libraries(riscv-vp ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

subdirs(sifive)