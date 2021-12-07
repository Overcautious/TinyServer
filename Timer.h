//
// Created by zhou on 2021/12/7.
//

#ifndef TINYSERVER_TIMER_H
#define TINYSERVER_TIMER_H
#include "all.h"
using TimeoutCallback = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;

class HttpRequest;

class TimerNode {
private:
    TimeStamp m_exporedTime;
    TimeoutCallback c_callBack;
    bool m_delete;
public:
    TimerNode(const TimeStamp &when, const TimeoutCallback &cb): m_exporedTime(when), c_callBack(cb), m_delete(false){}
    ~TimerNode(){}

    void del(){
        m_delete = true;
    }

    bool isDeleted(){
        return m_delete;
    }

    void runCallBack(){
        c_callBack();
    }

    TimeStamp getExpiredTime() const{
        return m_exporedTime;
    }

};

struct cmp{
    bool operator()(TimerNode *rsh1, TimerNode *rsh2){
        assert(rsh1 != nullptr && rsh2 != nullptr);
        return rsh1->getExpiredTime() > rsh2->getExpiredTime();
    }
};

class Timer{
private:
    std::priority_queue<TimerNode*, std::vector<TimerNode*>, cmp> m_timerQueue;
    TimeStamp m_nowTime;
    std::mutex m_mtx;

public:
    Timer():m_nowTime(Clock::now()){}
    ~Timer(){}

    void updateTime(){
        m_nowTime = Clock::now();
    }

    void addTimerNode(HttpRequest *req, const int &time_out, const TimeoutCallback &cb);

    void delTimerNode(HttpRequest* req);

    void handleExpiredTimerNode();

    int getNearestExpiredTimerNode();
};

#endif //TINYSERVER_TIMER_H
