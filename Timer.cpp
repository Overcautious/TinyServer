//
// Created by zhou on 2021/12/7.
//

#include "Timer.h"
#include "HttpRequest.h"

void Timer::addTimerNode(HttpRequest *req, const int &time_out, const TimeoutCallback &cb) {
    std::unique_lock<std::mutex> lck(m_mtx);
    assert(req != nullptr);

    updateTime();
    TimerNode* time = new TimerNode(m_nowTime + MS(time_out), cb);
    m_timerQueue.push(time);

    if(req->getTimer() != nullptr){
        delTimerNode(req);
    }
    req->setTimer(time);
}

void Timer::delTimerNode(HttpRequest *req) {
    assert(req != nullptr);

    TimerNode * time = req->getTimer();
    if()
}
