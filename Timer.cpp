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

    //如果对于同一个request连续调用两次addtimer，则需要删除前一个定时器
    if(req->getTimer() != nullptr){
        delTimerNode(req);
    }
    req->setTimer(time);
}

void Timer::delTimerNode(HttpRequest *req) {
    assert(req != nullptr);

    TimerNode * time = req->getTimer();
    if(time == nullptr)
        return ;
    time->del();    // 如果直接删除，会使最大堆里priority_queue里的指针变成垂悬指针。正确的方法就是惰性删除

    req->setTimer(nullptr);
}

void Timer::handleExpiredTimerNode() {
    std::unique_lock<std::mutex> lck(m_mtx);    //上锁
    updateTime();

    while(!m_timerQueue.empty()){
        TimerNode* time = m_timerQueue.top();
        assert(time != nullptr);

        if(time->isDeleted()){
            std::cout<<"[Timer::handleExpiredTimerNode] timer= "
            << Clock::to_time_t(time->getExpiredTime()) << " is deleted" <<std::endl;
            m_timerQueue.pop();
            delete time;
            continue;
        }
        // 队列头部的都没有超时
        if((std::chrono::duration_cast<MS>(time->getExpiredTime() - m_nowTime )).count() > 0){
            std::cout<< "[Timer::handleExpiredTimerNode] there is no timeout TimerNode" << std::endl;
            return;
        }
        // 超时
        std::cout<< "[Timer::handleExpiredTimerNode] one TimerNode timeout" << std::endl;
        time->runCallBack();
        m_timerQueue.pop();
        delete time;

    }
}

int Timer::getNearestExpiredTimerNode() {
    std::unique_lock<std::mutex> lck(m_mtx);
    updateTime();
    int res = -1;
    while(!m_timerQueue.empty()){
        TimerNode * time = m_timerQueue.top();
        if(time->isDeleted()){
            m_timerQueue.pop();
            delete time;
            continue;
        }
        res = std::chrono::duration_cast<MS>(time->getExpiredTime() - m_nowTime ).count();
        res = res<0 ? 0:res;
        return res;
    }

    return res;
}