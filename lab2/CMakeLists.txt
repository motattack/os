CMAKE_MINIMUM_REQUIRED(VERSION 3.5.0)
PROJECT(ProccesLib)

ADD_LIBRARY(process_library STATIC source/process.cpp)

ADD_EXECUTABLE(main source/main.cpp)
target_link_libraries(main PRIVATE process_library)

ADD_EXECUTABLE(example source/example.c)