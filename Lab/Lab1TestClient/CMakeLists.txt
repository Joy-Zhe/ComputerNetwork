cmake_minimum_required(VERSION 3.25)
project(Lab1TestClient C)

set(CMAKE_C_STANDARD 11)
link_libraries(ws2_32 wsock32)

add_executable(Lab1TestClient main.c)
