#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
using namespace std;
/*
 * КАК ВООБЩЕ РАБОТАЕТ СИНХРОНИЗАЦИЯ НА АТОМИКАХ?
 * Есть 3 отношения: Happens Before, Sync With, Interthreaded happens before
 * 1. Happens Before отношение между выражениями A и B происходит в одном потоке, если A следует до B
 * (про переупорядочивание пока забудем)
 * 2. Sync With отношение возникает ТОЛЬКО между ДОЛЖНЫМ ОБРАЗОМ маркированными операциями на ОДНОЙ атомарной переменной
 * 3. Если операция A в потоке1 Sync With B в другом, то A Interthreaded happens before B
 */

// Пример 1
//  Операция 2 Sync With ОПЕРАЦИЯ 3
// А так как ОПЕРАЦИЯ 1 Happens Before ОПЕРАЦИЯ 2
// (благодаря неявному полному барьеру или LS + SS барьеру у memory_order_release)
// То транзитивно ОПЕРАЦИЯ 1 Sync With ОПЕРАЦИЯ 4
void example1(){
    atomic<bool> data_ready{false};
    vector<int> data;
    auto write = [&data_ready, &data](){
        data.push_back(123); // ОПЕРАЦИЯ 1
        data_ready.store(true, memory_order_release); // ОПЕРАЦИЯ 2
    };

    auto read = [&data_ready, &data](){
        while(!data_ready.load(memory_order_acquire)){// ОПЕРАЦИЯ 3
            this_thread::sleep_for(std::chrono::microseconds (1));
        }
        cout<<data[0]<<endl; // ОПЕРАЦИЯ 4
        assert(data[0] == 123);
    };
    thread readThread(read);
    thread writeThread(write);
    readThread.join();
    writeThread.join();
}


using namespace std;

int main() {
    for(int i = 0; i < 1000000; ++i)
        example1();
    return 0;
}