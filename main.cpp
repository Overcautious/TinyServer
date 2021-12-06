#include <iostream>
#include <thread>
#include <vector>
#include "ThreadPool.h"

void fun(){
    int cnt = 0;
    while(cnt < 3){
        printf("This is job func %lu", unsigned long(std::this_thread::get_id())  );
        cnt++;
    }
};

int main() {
    std::cout << "Hello, World!" << std::endl;

    ThreadPool m_pool(8);
    int cnt = 0;
    while(cnt < 3){
        m_pool.pushJob(fun);
        cnt++;
    }

    std::cout << "All threads joined.\n";
    return 0;
}
