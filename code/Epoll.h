//
// Created by zhou on 2021/12/10.
//

#ifndef TINYSERVER_EPOLL_H
#define TINYSERVER_EPOLL_H
#include "all.h"

#define MAXEVENTS 1024


namespace TinyServer{

    class HttpRequest;
    class ThreadPool;

class Epoll {
public:
    using NewConnectionCallbak = std::function<void()>;
    using CloseConnectionCallbak = std::function<void(HttpRequest*)>;
    using HandleRequestCallbak = std::function<void(HttpRequest*)>;
    using HandleResponseCallbak = std::function<void(HttpRequest*)>;

    // function
    Epoll();
    ~Epoll();

    int wait(int timeoutMS);    //
    void handleEvent(int lsnFd, std::shared_ptr<ThreadPool>& threadPool, int eventsNum);

    // 修改监听事件
    int add(int fd, HttpRequest* req, int evnts);
    int mod(int fd, HttpRequest* req, int evnts);
    int del(int fd, HttpRequest* req, int evnts);

    void setConnectionCallbak(const NewConnectionCallbak& cb){
        c_buildConnection = cb;
    }

    void setCloseconnectionCallbak(const CloseConnectionCallbak& cb){
        c_closeConnection = cb;
    }

    void setRequest(const HandleRequestCallbak& cb){
        c_request = cb;
    }

    void setResponse(const HandleResponseCallbak& cb){
        c_response = cb;
    }

private:
    using m_eventList = std::vector<struct epoll_event>;
    int m_epollFd;
    m_eventList m_events;

    NewConnectionCallbak c_buildConnection;
    CloseConnectionCallbak c_closeConnection;
    HandleRequestCallbak  c_request;
    HandleResponseCallbak c_response;
};

}
#endif //TINYSERVER_EPOLL_H
