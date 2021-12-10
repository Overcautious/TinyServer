//
// Created by zhou on 2021/12/10.
//

#include "Server.h"
#include "Utils.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "Epoll.h"
#include "ThreadPool.h"
#include "Timer.h"

namespace TinyServer{
Server::Server(u_short port, int numThread)
        :m_port(port), m_lsnFd(Utils::createListenFd(m_port)),
        m_lsnReq(static_cast<HttpRequest *>(new HttpRequest(m_lsnFd))), m_epoll(new Epoll()),
        m_threadPool(new ThreadPool(numThread)), m_timer(new Timer()){
    assert(m_lsnFd >= 0);
}

Server::~Server() {}



void Server::run() {
    m_epoll->add(m_lsnFd, m_lsnReq.get(), (EPOLLIN | EPOLLET));
    m_epoll->setConnectionCallbak(std::bind(&Server::acceptConnection, this));
    m_epoll->setCloseconnectionCallbak(std::bind(&Server::closeConnection, this, std::placeholders::_1));
    m_epoll->setRequest(std::bind(&Server::doRequest, this, std::placeholders::_1));
    m_epoll->setResponse(std::bind(&Server::doResponse, this, std::placeholders::_1));

    while(true){
        int timeDiff = m_timer->getNearestExpiredTimerNode();
        int eventsNum = m_epoll->wait(timeDiff);
        if(eventsNum > 0){
            m_epoll->handleEvent(m_lsnFd, m_threadPool, eventsNum);
        }
        m_timer->handleExpiredTimerNode();
    }
}

void Server::closeConnection(HttpRequest* req) {
    int fd = req->getFd();
    if(req->isWorking())
        return;
    m_timer->delTimerNode(req);
    m_epoll->del(fd, req, 0);
    delete req;
    req = nullptr;
}

void Server::acceptConnection() {
    while(true){
        int acceptFd = ::accept4(m_lsnFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if(acceptFd == -1){
            if(errno == EAGAIN)
                break;
            printf("[Server::acceptConnection] accept: %s\n", strerror(errno));
            break;
        }

        HttpRequest* req = new HttpRequest(acceptFd);
        m_timer->addTimerNode(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        m_epoll->add(acceptFd, req, (EPOLLIN | EPOLLONESHOT));
    }
}

void Server::doRequest(HttpRequest *req) {
    m_timer->delTimerNode(req);
    assert(req != nullptr);
    int fd = req->getFd();

    int rdErrno;    // errno码
    int retRead = req->readData(&rdErrno);

    if(retRead == 0){
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if(retRead < 0 && rdErrno != EAGAIN){
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if(retRead < 0 && rdErrno == EAGAIN){
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLONESHOT));
        req->setNoWorking();
        m_timer->addTimerNode(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        return;
    }

    if(!req->parseRequest()){
        HttpResponse response(400, "", false);
        req->appendOutBuffer(response.makeResponse());
        int wrErrno;
        req->writeData(&wrErrno);
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if(req->parseFinish()){
        HttpResponse response(200, req->getPath(), req->keepAlive());
        req->appendOutBuffer(response.makeResponse());
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    }
}

void Server::doResponse(HttpRequest *req) {
    m_timer->delTimerNode(req); // ?
    assert(req != nullptr);
    int fd = req->getFd();
    int bytesToWrite = req->writableBytes();
    if(bytesToWrite == 0){
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLONESHOT));
        req->setNoWorking();
        m_timer->addTimerNode(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        return;
    }

    int wrErrno;
    int retWrite = req->writeData(&wrErrno);

    if(retWrite < 0 && wrErrno == EAGAIN){
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
        return;
    }

    if(retWrite < 0 && wrErrno != EAGAIN){
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if(retWrite == bytesToWrite){
        if(req->keepAlive()){
            req->resetParse();
            m_epoll->mod(fd, req, (EPOLLIN | EPOLLONESHOT));
            req->setNoWorking();
            m_timer->addTimerNode(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        }else{
            req->setNoWorking();
            closeConnection(req);
        }
        return;
    }

    m_epoll->mod(fd, req, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    req->setNoWorking();
    m_timer->addTimerNode(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
    return;

}

}   // end namespace
