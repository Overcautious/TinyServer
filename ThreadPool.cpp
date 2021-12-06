//
// Created by zy on 2021/12/5.
//

#include "ThreadPool.h"


#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
ThreadPool::ThreadPool(int numWokers): m_isClose(false) {
    numWokers = numWokers <= 0 ? 1:numWokers;

    for(int i=0; i<numWokers; i++){
        m_threads.emplace_back([this]{      // 在仿函数内部使用了
            while(1){
                std::unique_lock<std::mutex> lck(m_mtx);
                while( !m_isClose  && m_jobs.empty() ){
                    m_cond.wait(lck);
                }
                if(m_jobs.empty() && m_isClose){
                    // std::cout << "[ThreadPool::TheadPool] thread_id = " << std::this_thread::get_id() << "return" << std::endl; // 非线程安全
                    printf("[ThreadPool::TheadPool] thread_id = %lu return\n", std::this_thread::get_id());
                    return;
                }

                JobFunction job = m_jobs.front();
                m_jobs.pop();

                if(job){
                    printf("[ThreadPool::ThreadPool] thread_id = %lu get a job\n", std::this_thread::get_id());
                    job();
                    printf("[ThreadPool::ThreadPool] thread_id = %lu job finish\n", std::this_thread::get_id());
                }
            }

        });
    }
}

ThreadPool::~ThreadPool() {
    std::unique_lock<std::mutex> lck(m_mtx);
    m_isClose = true;

    m_cond.notify_all();
    for (auto &thread : m_threads)
    {
        thread.join();
    }
    printf("[ThreadPool::~ThreadPool] threadpool is remove\n");
}

void ThreadPool::pushJob(const JobFunction &job)
{
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_jobs.push(job);
    }

    printf("[ThreadPool::pushJob] push new job\n");
    m_cond.notify_one();
}

#pragma clang diagnostic pop