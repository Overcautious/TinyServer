//
// Created by zhou on 2021/12/10.
//

#ifndef TINYSERVER_UTILS_H
#define TINYSERVER_UTILS_H
#include "all.h"
namespace TinyServer{
namespace Utils {

    int createListenFd(u_short port);
    int setNonBlocking(int fd);

};

}
#endif //TINYSERVER_UTILS_H
