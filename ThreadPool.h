//
// Created by zy on 2021/12/5.
//

#ifndef TINYSERVER_THREADPOOL_H
#define TINYSERVER_THREADPOOL_H
#include "all.h"

class ThreadPool {
public:
    using JobFunction = std::function<void()>;

    ThreadPool(int numWokers);
    ~ThreadPool();

    void pushJob(const JobFunction& job);

private:
    std::vector<std::thread> m_threads;
    std::mutex m_mtx;
    std::condition_variable m_cond;
    std::queue<JobFunction> m_jobs;
    bool m_isClose;
};


#endif //TINYSERVER_THREADPOOL_H
