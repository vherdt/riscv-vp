cmake_minimum_required(VERSION 2.8.12)
project(riscv-vp)

ADD_DEFINITIONS(-std=c++14 -g3)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package( Boost REQUIRED COMPONENTS iostreams program_options )

find_package( SystemC REQUIRED )
include_directories( ${SystemC_INCLUDE_DIRS} )

subdirs(lib)
