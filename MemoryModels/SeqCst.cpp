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
void example1() {
    cout << "EXAMPLE1:" << endl;
    atomic<bool> data_ready{false};
    vector<int> data;
    auto write = [&data_ready, &data]() {
        data.push_back(123); // ОПЕРАЦИЯ 1
        data_ready.store(true); // ОПЕРАЦИЯ 2
        // data_ready.store(true, memory_order_release); // эффект тот же

    };
    auto read = [&data_ready, &data]() {
        // while(!data_ready.load(memory_order_acquire)){// ОПЕРАЦИЯ 3
        while (!data_ready.load()) {
            //this_thread::sleep_for(std::chrono::microseconds(1));
        }
        cout << "data[0] == " << data[0] << endl; // ОПЕРАЦИЯ 4
        assert(data[0] == 123);
    };
    thread readThread(read);
    thread writeThread(write);
    readThread.join();
    writeThread.join();
}


// Хоть один да выстрелит
// это все из за глобального порядка операций на атомиках
//
int incrz() {
    std::atomic<bool> x{false};
    std::atomic<bool> y{false};
    std::atomic<int> z{0};
    auto writeX = [&]() {
        x.store(true);
    };

    auto writeY = [&]() {
        y.store(true);
    };

    auto readXThenY = [&]() {
        while (!x.load()) {}
        if (y.load())
            ++z;
    };

    auto readYThenX = [&]() {
        while (!y.load()) {}
        if (x.load())
            ++z;
    };

    thread t1(writeX);
    thread t2(writeY);
    thread t3(readXThenY);
    thread t4(readYThenX);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    assert(z != 0);
    return z.load();
}

void example2() {
    cout << "EXAMPLE2" << endl;
    int zis1 = 0;
    int zis2 = 0;
    int itreations = 100000;
    for (int i = 0; i < itreations; ++i) {
        int z = incrz();
        if (z == 0)
            throw;
        if (z == 1)
            ++zis1;
        else if (z == 2)
            ++zis2;
    }
    cout << "Z == 1 " << zis1 << " times" << endl;
    cout << "Z == 2 " << zis2 << " times" << endl;
}


int main() {
    example2();
    return 0;
}