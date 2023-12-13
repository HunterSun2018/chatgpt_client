cmake_minimum_required(VERSION 3.0)

project(test)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

include_directories(~/boost_1_81_0)

# Library http_client
add_library(co_http_client SHARED src/co_http_client.cpp)

target_link_libraries(co_http_client pthread ssl crypto) #crypto

include_directories( src/)

# Executable tests
add_executable(tests tests/main.cpp)

target_link_libraries(tests co_http_client gtest)

# Install all targets
install(TARGETS co_http_client)
set(PUBLIC_HEADER src/co_helper.hpp src/co_http_client.hpp)
install(FILES ${PUBLIC_HEADER} DESTINATION include)