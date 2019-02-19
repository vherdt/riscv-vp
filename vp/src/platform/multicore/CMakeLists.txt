file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(riscv-mvp
        mc_main.cpp
        ${HEADERS})

target_link_libraries(riscv-mvp rv32 platform-common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

INSTALL(TARGETS riscv-mvp RUNTIME DESTINATION bin)