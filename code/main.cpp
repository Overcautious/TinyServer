#include <iostream>
#include <thread>
#include <vector>
#include "ThreadPool.h"
#include "Server.h"

int main() {
    u_short port = 8080;

    int numThread = 4;

    TinyServer::Server server(port, numThread);
    server.run();

    return 0;
}
