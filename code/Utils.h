//
// Created by zhou on 2021/12/10.
//

#ifndef TINYSERVER_UTILS_H
#define TINYSERVER_UTILS_H
#include "all.h"
#define LSTNQUE 1024 // 监听队列长度,操作系统默认值为SOMAXCONN
namespace TinyServer{

namespace Utils {

    int createListenFd(u_short port);
    int setNonBlocking(int fd);

};

}
#endif //TINYSERVER_UTILS_H
