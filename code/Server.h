//
// Created by zhou on 2021/12/10.
//

#ifndef TINYSERVER_SERVER_H
#define TINYSERVER_SERVER_H
#include "all.h"

namespace TinyServer
{

class HttpRequest;
class Epoll;
class ThreadPool;
class Timer;

class Server {
private:
    using RequestPtr = std::unique_ptr<HttpRequest>;
    using EpollPtr = std::unique_ptr<Epoll>;
    using ThreadPoolPtr = std::shared_ptr<ThreadPool>;
    using TimerPtr = std::unique_ptr<Timer>;

public:
    Server(u_short port, int numThread);
    ~Server();
    void run();

private:
    void acceptConnection();
    void closeConnection(HttpRequest* req);
    void doRequest(HttpRequest* req);
    void doResponse(HttpRequest* req);

    // 成员
    int m_port;
    int m_lsnFd;

    RequestPtr m_lsnReq;
    EpollPtr m_epoll;
    ThreadPoolPtr m_threadPool;
    TimerPtr m_timer;

};
}

#endif //TINYSERVER_SERVER_H
