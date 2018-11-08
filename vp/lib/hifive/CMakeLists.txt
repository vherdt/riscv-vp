add_executable(hifive-vp
        hifive_main.cpp
        ../syscall.cpp
        ../instr.cpp
        gpio/gpiocommon.cpp
        gpio/gpio-server.cpp
        ${HEADERS})

target_include_directories(hifive-vp PRIVATE ..)

target_link_libraries(hifive-vp ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)
