file(GLOB_RECURSE HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_executable(linux-vp
        linux_main.cpp
        ${HEADERS})

if(GDB_MULTICORE)
	target_link_libraries(linux-vp rv64 platform-common gdb-mc
		${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)

	target_compile_definitions(linux-vp PRIVATE -DGDB_MULTICORE)
else()
	target_link_libraries(linux-vp rv64 platform-common
		${Boost_LIBRARIES} ${SystemC_LIBRARIES} pthread)
endif()

INSTALL(TARGETS linux-vp RUNTIME DESTINATION bin)
