cmake_minimum_required(VERSION 3.20)
project(ProccesLib)

set(CMAKE_CXX_STANDARD 20)

add_library(process_library STATIC source/process.cpp)

add_executable(main source/main.cpp)
target_link_libraries(main PRIVATE process_library)

add_executable(example source/example.c)