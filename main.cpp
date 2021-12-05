#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::cout << "Hello, World!" << std::endl;

    std::vector<std::thread> m_thread;

    for(int i=0; i<4; i++)
    m_thread.emplace_back([i](){
        int cnt = 0;
        while(1) {
            std::this_thread::sleep_for(std::chrono::seconds(i));
            std::cout << "hello thread "
                      << std::this_thread::get_id()
                      << " paused " << i << " seconds" << std::endl;
            if(cnt++ > 3)
                break;
        }
    });

    std::cout << "Done spawning threads! Now wait for them to join\n";
    for (auto& t: m_thread) {
        t.join();
    }
    std::cout << "All threads joined.\n";
    return 0;
}
