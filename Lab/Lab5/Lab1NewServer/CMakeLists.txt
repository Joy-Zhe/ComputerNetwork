cmake_minimum_required(VERSION 3.25)
project(Lab1NewServer)

set(CMAKE_CXX_STANDARD 17)

link_libraries(ws2_32 wsock32)

add_executable(Lab1NewServer main.cpp server.h parser.h)
