cmake_minimum_required(VERSION 3.20)
project(lab5)

set(CMAKE_CXX_STANDARD 20)

add_executable(main source/main.cpp source/sqlite3.c)
add_executable(simulacron source/simulacron.cpp)
add_executable(server source/server.cpp source/sqlite3.c)

IF (WIN32)
    TARGET_LINK_LIBRARIES(server ws2_32)
ENDIF(WIN32)