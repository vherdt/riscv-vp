add_executable(sifive-vp
        sifive_main.cpp
        ../syscall.cpp
        ../instr.cpp
        ${HEADERS})

target_include_directories(sifive-vp PRIVATE ..)

target_link_libraries(sifive-vp ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)
