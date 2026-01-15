cmake_minimum_required(VERSION 3.10)
project(bmssp_solver)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -O3")

# Main executable
add_executable(bmssp_solver main.cpp bmssp.cpp block_list.cpp)

# Test executable
add_executable(test_block_list test_block_list.cpp block_list.cpp)

# Enable testing
enable_testing()

# Add test
add_test(NAME BlockListTest COMMAND test_block_list)
