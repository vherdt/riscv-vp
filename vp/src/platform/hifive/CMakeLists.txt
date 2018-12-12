file(GLOB SOURCES *.h *.cpp)

add_executable(hifive-vp
        hifive_main.cpp
        gpio.cpp
        gpio/gpiocommon.cpp
        gpio/gpio-server.cpp
        ${COMMON_HEADERS}
        ${COMMON_SOURCES})

##target_include_directories(hifive-vp PRIVATE ..)

target_link_libraries(hifive-vp rv32 common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)
