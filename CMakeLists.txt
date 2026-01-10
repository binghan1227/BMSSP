cmake_minimum_required(VERSION 3.10)
project(bmssp_solver)

set(CMAKE_CXX_STANDARD 17)

add_executable(bmssp_solver main.cpp bmssp.cpp block_list.cpp)
