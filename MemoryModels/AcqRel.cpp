#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
using namespace std;

// ПРИМЕР как из SeqCst
// Теперь и 0 и 1 и 2 возможен
// так как синхронизация между  x.store(true, memory_order_release) и x.load(memory_order_acquire) (или y симетрично)
// никак не влияет на Y
int incrz() {
    std::atomic<bool> x{false};
    std::atomic<bool> y{false};
    std::atomic<int> z{0};
    auto writeX = [&]() {
        x.store(true, memory_order_release);
    };

    auto writeY = [&]() {
        y.store(true, memory_order_release);
    };

    auto readXThenY = [&]() {
        while (!x.load(memory_order_acquire)) {}
        if (y.load(memory_order_acquire))
            ++z;
    };

    auto readYThenX = [&]() {
        while (!y.load(memory_order_acquire)) {}
        if (x.load(memory_order_acquire))
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

void example1() {
    cout << "EXAMPLE1" << endl;
    int zis0 = 0;
    int zis1 = 0;
    int zis2 = 0;
    int itreations = 100000;
    for (int i = 0; i < itreations; ++i) {
        int z = incrz();
        if (z == 0)
            ++zis0;
        else if (z == 1)
            ++zis1;
        else if (z == 2)
            ++zis2;
    }
    cout << "Z == 0 " << zis1 << " times" << endl;
    cout << "Z == 1 " << zis1 << " times" << endl;
    cout << "Z == 2 " << zis2 << " times" << endl;

    // на 100 000 раз
    // Z == 0 71 times
    // Z == 1 71 times
    // Z == 2 99929 times
}

// Теперь рассмотри example1 из Relexed
// в writeXY x.store(true, memory_order_release); создает барьер LS + SS и как толькр сработал
// x.load(memory_order_acquire) Y уже синхронизированн
int incrzFromRelexed() {
    std::atomic<bool> x{false};
    std::atomic<bool> y{false};
    std::atomic<int> z{0};
    auto writeXY = [&]() {
        y.store(true, memory_order_relaxed);
        x.store(true, memory_order_release);
    };

    auto readXThenY = [&]() {
        while (!x.load(memory_order_acquire)) {}
        if (y.load(memory_order_relaxed))
            ++z;
    };

    thread t1(writeXY);
    thread t2(readXThenY);
    t1.join();
    t2.join();
    return z.load();
}

void example1FromRelexed() {
    cout << "example1FromRelexed" << endl;
    int zis0 = 0;
    int zis1 = 0;
    int itreations = 100000;
    for (int i = 0; i < itreations; ++i) {
        int z = incrzFromRelexed();
        if (z == 0)
            ++zis0;
        if (z == 1)
            ++zis1;
    }
    cout << "Z == 0 " << zis0 << " times" << endl;
    cout << "Z == 1 " << zis1 << " times" << endl;
}

// Теперь синхронизируем транзитивно 3 потока

vector<int> threeThreads2Atomics(){
    const int size = 5;
    int data[size] = {};
    atomic<bool> sync1;
    atomic<bool> sync2;
    auto incr1 = [&](){
        for(int i = 0; i < size; ++i)
            data[i] = i + 1;
        sync1.store(true, memory_order_release);
    };
    auto incr2 = [&](){
        while(!sync1.load(memory_order_acquire));
        for(int i = 0; i < size; ++i)
            data[i] += data[i] + i;
        sync2.store(true, memory_order_release);
    };

    vector<int> res;
    auto fillRes = [&](){
        while(!sync2.load(memory_order_acquire));
        for(auto n: data)
            res.push_back(n);
    };
    thread t1(incr1);
    thread t2(incr2);
    thread t3(fillRes);
    t1.join();
    t2.join();
    t3.join();
    return res;
}
// На 1м атомике
vector<int> threeThreads1Atomic(){
    atomic<int> sync{0};
    const int size = 5;
    int data[size] = {};
    auto incr1 = [&](){
        for(int i = 0; i < size; ++i)
            data[i] = i + 1;
        sync.store(1, memory_order_release);
    };

    auto incr2 = [&](){
        int expected = 1;
        while(sync.compare_exchange_strong(expected, 2, memory_order_acq_rel));
        for(int i = 0; i < size; ++i)
            data[i] += data[i] + i;
        sync.fetch_add(1, memory_order_release);
    };
    vector<int> res;
    auto fillRes = [&](){
        int expected = 3;
        while(sync.load(memory_order_acquire) != 3);
        for(auto n: data)
            res.push_back(n);
    };
    thread t1(incr1);
    thread t2(incr2);
    thread t3(fillRes);
    t1.join();
    t2.join();
    t3.join();
    return res;
}

void threeThreads(){
    int itreations = 100000;
    for (int i = 0; i < itreations; ++i) {
        auto res1 = threeThreads2Atomics();
        auto res2 = threeThreads1Atomic();
        if(res1 != res2)
            throw;
    }
}


int main(){
    threeThreads();
}