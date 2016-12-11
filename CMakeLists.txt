cmake_minimum_required(VERSION 3.6)
project(Forwarder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp connection_list.h)
add_executable(Forwarder ${SOURCE_FILES})