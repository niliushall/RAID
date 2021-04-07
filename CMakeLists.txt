cmake_minimum_required(VERSION 2.8)
project(RAID)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)
add_subdirectory(src)

add_executable(main main.cpp src/FileDisk.cpp include/FileDisk.h)
target_link_libraries(main SRC)