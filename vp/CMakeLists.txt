cmake_minimum_required(VERSION 2.8.12)
project(riscv-vp)

set(CMAKE_CXX_STANDARD 14) # This is equal to QMAKE_CXX_FLAGS += -std=c++xxxx

set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package( Boost REQUIRED COMPONENTS iostreams program_options )

find_package( SystemC REQUIRED )
include_directories( ${SystemC_INCLUDE_DIRS} )

subdirs(src)
