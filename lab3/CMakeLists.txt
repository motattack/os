cmake_minimum_required(VERSION 3.20)
project(lab3)

set(CMAKE_CXX_STANDARD 20)

ADD_LIBRARY(process_library STATIC source/process.cpp)

add_executable(main source/main.cpp)
target_link_libraries(main PRIVATE process_library)