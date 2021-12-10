cmake_minimum_required(VERSION 3.10)
project(TinyServer)

set(CMAKE_CXX_STANDARD 11)

link_directories( /usr/lib/x86_64-linux-gnu/libpthread.so )
include_directories( /usr/lib/x86_64-linux-gnu/libpthread.a )

add_executable(TinyServer main.cpp ThreadPool.cpp ThreadPool.h all.h Buffer.cpp Buffer.h Timer.cpp Timer.h HttpRequest.cpp HttpRequest.h HttpResponse.cpp HttpResponse.h Utils.cpp Utils.h Epoll.cpp Epoll.h Server.cpp Server.h)

target_link_libraries(TinyServer pthread)