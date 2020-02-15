#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
using namespace std;

// RMW операции:
// RMW-операция загружает ПОСЛЕДНЕЕ значение переменной. Даже _relexed
// Они являются ОСНОВОЙ для lock-free программирования
int relexedIncr(){
    atomic<int> counter{0};
    auto incr = [&](){
        counter.fetch_add(1, memory_order_relaxed);
    };
    vector<thread> incrementThreads;
    incrementThreads.reserve(10);
    for(int i = 0; i < 10; ++i)
        incrementThreads.emplace_back(incr);
    for(auto &t: incrementThreads)
        t.join();
    return counter.load();
}

void RMW_power(){
    for(int i = 0; i < 100000; ++i){
        int res = relexedIncr();
        assert(res == 10);
    }
}

// SpinLock из книжки
class SpinLock
{
    std::atomic_flag locked;
public:
    SpinLock() :
            locked{ATOMIC_FLAG_INIT}
    {
    }
    void lock()
    {
        // memory_order_acquire, а не acq_rel, потому что acq_rel для RMW ИЗБЫТОЧЕН,
        // acq_rel нужен ТОЛЬКО если мы на нем какую то синхронизацию строим
        while(locked.test_and_set(std::memory_order_acquire));
    }
    void unlock()
    {
        locked.clear(std::memory_order_release);
    }
};

// Кто такой этот ваш Release-Seq ???
// Если (A) это атомарный store в x c моделью памяти (release, acq_rel или seq_cst) Это ОСВОБОЖДЕНИЕ
// И (B)  это атомарный load из x с моделью памяти( acq или seq_cst) это ЗАХВАТ
// 1. Между ними может идти ЛЮБОЕ количество RMW - операций ИМЕННО RMW, так как ЛЮБАЯ
// RMW-операция загружает ПОСЛЕДНЕЕ значение переменной. Даже _relaxed
// 2. Так же в потоке с первоначальным store любое кол во store после 1го допустимо

void producerConsumers(){
    const int N = 10000;
    atomic<bool> stop{false};
    vector<int> data;
    atomic<int> dataSize{-1};

    auto producer = [&](){
        for(int i = 0; i < N; ++i){
            data.push_back(i);
        }
        dataSize.store(N-1, memory_order_release);
    };

    auto consumer = [&](int id){
        while(true){
            int dataIndex = -1;
            while(dataIndex < 0){
                if(stop.load(memory_order_relaxed))
                    break;
                dataIndex = dataSize.fetch_sub(1, memory_order_acquire);
            }
            if(stop.load(memory_order_relaxed))
                break;
            //cout<<"TREAD: "<<id<<" dataIndex=="<<dataIndex<<endl;
            //cout<<id<<' '<<dataIndex<<endl;
            int num = data[dataIndex];
            //cout<<"TREAD: "<<id<<" num=="<<num<<endl;

            if(dataIndex == 0){
                stop.store(true, memory_order_relaxed);
                break;
            }
        }
    };

    thread t1(producer);
    thread t2(consumer, 1);
    thread t3(consumer, 2);
    t1.join();
    t2.join();
    t3.join();
}


int main(){
    int c = 0;
    while(true){
        cout<<"ITERATION=="<<c<<endl;
        producerConsumers();
        ++c;
    }
}