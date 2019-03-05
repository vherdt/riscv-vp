file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(mc4-vp
        mc4_main.cpp
        ${HEADERS})

target_link_libraries(mc4-vp rv32 platform-common ${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

INSTALL(TARGETS mc4-vp RUNTIME DESTINATION bin)