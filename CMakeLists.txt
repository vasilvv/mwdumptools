cmake_minimum_required(VERSION 2.8)

project (mwdumptools)

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -ggdb -std=c++11")
endif()

find_package(ZLIB)
find_package(EXPAT)
find_package(LibArchive)

add_subdirectory(src)
