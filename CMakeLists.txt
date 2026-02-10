cmake_minimum_required(VERSION 3.10)
project(bmssp_solver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O3")

option(BMSSP_ENABLE_TRACE "Enable algorithm tracing" OFF)
if(BMSSP_ENABLE_TRACE)
    add_compile_definitions(BMSSP_TRACE)
endif()

# Main executable
add_executable(bmssp_solver main.cpp bmssp.cpp block_list.cpp)

# Dijkstra baseline executable
add_executable(dijkstra_solver dijkstra.cpp)

# Test executable
add_executable(test_block_list test_block_list.cpp block_list.cpp)

# Enable testing
enable_testing()

# Add test
add_test(NAME BlockListTest COMMAND test_block_list)
