#include <iostream>
#include <atomic>
#include <thread>

using namespace std;
// Хоть один да выстрелит
// это все из за глобального порядка операций на атомиках
//
int incrz() {
    std::atomic<bool> x{false};
    std::atomic<bool> y{false};
    std::atomic<int> z{0};
    auto writeXY = [&]() {
        x.store(true, memory_order_relaxed);
        y.store(true, memory_order_relaxed);
    };

    auto readXThenY = [&]() {
        while (!x.load(memory_order_relaxed)) {}
        if (y.load(memory_order_relaxed))
            ++z;
    };

    thread t1(writeXY);
    thread t2(readXThenY);
    t1.join();
    t2.join();
    return z.load();
}

void example1() {
    cout << "EXAMPLE1" << endl;
    int zis0 = 0;
    int zis1 = 0;
    int itreations = 100000;
    for (int i = 0; i < itreations; ++i) {
        int z = incrz();
        if (z == 0)
            ++zis0;
        if (z == 1)
            ++zis1;
    }
    cout << "Z == 0 " << zis0 << " times" << endl;
    cout << "Z == 1 " << zis1 << " times" << endl;
    // на 100 000 раз
//    Z == 0 3 times
//    Z == 1 99997 times

}

int main() {
    example1();
}