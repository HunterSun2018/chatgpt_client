cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)
set(CMAKE_CXX_FLAGS -fcoroutines)

add_executable(test src/main.cpp)

target_link_libraries(test pthread cpprest ssl crypto)