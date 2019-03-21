add_executable(hifive-vp
        hifive_main.cpp
        can.cpp
        gpio.cpp
        gpio/gpiocommon.cpp
        gpio/gpio-server.cpp
        ${HEADERS})

target_link_libraries(hifive-vp rv32 platform-common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

INSTALL(TARGETS hifive-vp RUNTIME DESTINATION bin)
