add_executable(hifive-vp
        hifive_main.cpp
        gpio.cpp
        gpio/gpiocommon.cpp
        gpio/gpio-server.cpp
        ${HEADERS})

target_include_directories(hifive-vp PRIVATE ..)

target_link_libraries(hifive-vp rv32 common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)
