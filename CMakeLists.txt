cmake_minimum_required(VERSION 3.10)

project(Alloy VERSION 0.1)

# Set the compiler to use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_library(Alloy INTERFACE)

target_include_directories(Alloy INTERFACE Source Include)
