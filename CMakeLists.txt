cmake_minimum_required(VERSION 3.17)
project(TinyServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(TinyServer main.cpp ThreadPool.cpp ThreadPool.h all.h)