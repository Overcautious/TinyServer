cmake_minimum_required(VERSION 3.10)
project(TinyServer)

set(CMAKE_CXX_STANDARD 11)

link_directories( /usr/lib/x86_64-linux-gnu/libpthread.so )
include_directories( /usr/lib/x86_64-linux-gnu/libpthread.a )

add_executable(TinyServer code/main.cpp code/ThreadPool.cpp code/ThreadPool.h code/all.h code/Buffer.cpp code/Buffer.h code/Timer.cpp code/Timer.h code/HttpRequest.cpp code/HttpRequest.h code/HttpResponse.cpp code/HttpResponse.h code/Utils.cpp code/Utils.h code/Epoll.cpp code/Epoll.h code/Server.cpp code/Server.h)

target_link_libraries(TinyServer pthread)