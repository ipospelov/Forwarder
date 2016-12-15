cmake_minimum_required(VERSION 3.6)
project(Forwarder)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    connection_list.h
    main.cpp)

add_executable(Forwarder ${SOURCE_FILES})