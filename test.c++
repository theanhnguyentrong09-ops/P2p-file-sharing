#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::thread t([]() {
        for(int i=0;i<5;i++){
            std::cout << "Thread running: " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });

    std::cout << "Main thread continues\n";
    t.detach();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Main ends\n";
    return 0;
}
