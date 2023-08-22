cmake_minimum_required(VERSION 3.0)

project(test)

#set(CMAKE_CXX_COMPILER g++-12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

include_directories(~/boost_1_81_0)
#link_directories(~/boost_1_81_0/)

# Library http_client
add_library(http_client SHARED src/http_client.cpp)

target_link_libraries(http_client pthread ssl crypto) #crypto

include_directories( src/)

# Executable client
add_executable(chat src/main.cpp)

target_link_libraries(chat http_client readline)

# Executable tests
add_executable(tests tests/main.cpp)

target_link_libraries(tests http_client gtest)

# Install all targets
install(TARGETS http_client chat )
set(PUBLIC_HEADER src/co_helper.hpp src/http_client.hpp)
install(FILES ${PUBLIC_HEADER} DESTINATION include)