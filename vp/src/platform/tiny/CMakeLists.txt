file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(tiny-vp
        tiny_main.cpp
        ${HEADERS})

target_link_libraries(tiny-vp rv32 platform-common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

INSTALL(TARGETS tiny-vp RUNTIME DESTINATION bin)