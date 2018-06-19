file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(riscv-vp
        main.cpp
        syscall.cpp
        gdb_stub.cpp
        ${HEADERS})

target_link_libraries(riscv-vp ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)
