cmake_minimum_required(VERSION 3.24)
project(Lab1 C)

set(CMAKE_C_STANDARD 17)

link_libraries(ws2_32)

add_executable(Lab1 server.c)
