cmake_minimum_required(VERSION 3.10)
project(MyTool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(MyTool main.cpp)
target_include_directories(MyTool PRIVATE ${CMAKE_SOURCE_DIR}/include)
