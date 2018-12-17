file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(riscv-vp
        main.cpp
        ethernet.cpp
        display.cpp
        ${HEADERS})

target_link_libraries(riscv-vp rv32 platform-common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

INSTALL(TARGETS riscv-vp RUNTIME DESTINATION bin)