cmake_minimum_required(VERSION 3.25)
project(Server)

set(CMAKE_CXX_STANDARD 17)

link_libraries(ws2_32)

add_executable(Server main.cpp server.h parser.h)
