//
// Created by zhou on 2021/12/10.
//

#include "Utils.h"
namespace TinyServer{
int Utils::createListenFd(u_short port) {

    port = ((port <= 1204) || (port >= 65535)) ? 9877 : port;
    int lsnFd = 0;
    if((lsnFd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) == -1){
        printf("[Utils::createListenFd] lsnFd = %d socket: %s\n", lsnFd, strerror(errno));
        ::close(lsnFd);
        return -1;
    }

    int optVal = 1;
    if(::setsockopt(lsnFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optVal, sizeof(int)) == -1){
        printf("[Utils::createListenFd] lsnFd = %d setsockopt: %s\n", lsnFd, strerror(errno));
        ::close(lsnFd);
        return -1;
    }

    struct sockaddr_in addr;
    ::bzero((char*)&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = ::htons(port);

    if(::bind(lsnFd, (struct sockaddr*)&addr, sizeof(addr)) == -1){
        printf("[Utils::createListenFd] lsnFd = %d setsockopt: %s\n", lsnFd, strerror(errno));
        ::close(lsnFd);
        return -1;
    }

    if(lsnFd == -1){
        ::close(lsnFd);
        return  -1;
    }
    printf("[Utils::createListenFd] lsnFd = %d \n", lsnFd);
    return lsnFd;
}

int Utils::setNonBlocking(int fd) {
    int flag = fcntl(fd, F_GETFL, 0);
    if(flag == -1)
    {
        printf("[Utils::setNonBlocking] lsnFd = %d fcntl: %s\n", fd, strerror(errno));
        return -1;
    }

    flag |= O_NONBLOCK;
    if(fcntl(fd, F_SETFL, flag) == -1){
        printf("[Utils::setNonBlocking] lsnFd = %d fcntl: %s\n", fd, strerror(errno));
        return -1;
    }
    return 0;
}

}