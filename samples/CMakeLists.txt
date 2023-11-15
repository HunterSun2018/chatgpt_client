cmake_minimum_required(VERSION 3.17)

project(http-client-test)
set(CMAKE_CXX_STANDARD 20)

add_executable(bing-client bing_client/main.cpp)

target_link_libraries(bing-client http_client)

include_directories(~/boost_1_81_0)
# Executable client
add_executable(chat chatgpt_client/main.cpp)

target_link_libraries(chat http_client readline)