#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

using namespace std;

// Первая версия
namespace v1 {
    template<typename T>
    struct node {
        node(int data) : data(data), next(nullptr) {}

        T data;
        node *next;
    };

    template<typename T>
    struct Stack {
        atomic<node<T> *> head;

        void push(T data) {
            // все как в обычном push в список
            auto newNode = new node(data);
            newNode->next = head.load();
            // меняем голову
            while (!head.compare_exchange_weak(newNode->next, newNode));
        }

        // Проблемы:
        // 1. ячейки текут
        // 2. result = oldHead->data может кинуть исключение
        int pop(T &result) {
            auto *oldHead = head.load();
            // меняем голову
            while (oldHead && !head.compare_exchange_weak(oldHead, oldHead->next));
            if (oldHead)
                result = oldHead->data;
        }
    };

}

// Безопасный, но текущий вариант
namespace v1_1 {
    template<typename T>
    struct node {
        node(T data) : data(make_shared<T>(data)), next(nullptr) {}

        shared_ptr<T> data;
        node *next;
    };

    template<typename T>
    struct Stack {
        atomic<node<T> *> head{nullptr};

        void push(T data) {
            // все как в обычном push в список
            auto newNode = new node(data);
            newNode->next = head.load();
            // меняем голову
            while (!head.compare_exchange_weak(newNode->next, newNode));
        }

        shared_ptr<T> pop() {
            auto *oldHead = head.load();
            // меняем голову
            while (oldHead && !head.compare_exchange_weak(oldHead, oldHead->next));
            if (oldHead){
                // Почему нельзя написать этот код?
                // Если t1 прихранил oldHead, а t2 уго удалил,
                // то в t1 oldHead->next упадет

                // auto res = oldHead->data;
                // delete oldHead;
                // oldHead = nullptr;
                // return res;
                return oldHead->data;
            }
            return nullptr;
        }
    };

}

int main() {
    v1_1::Stack<string> testStack;
    auto producer = [&testStack](){
        string prefix = "name_";
        for(int i = 0; i < 1000; ++i)
            testStack.push(prefix + to_string(i));
    };

    auto consumer = [&testStack](const int id){
        for(int i = 0; i < 1000; ++i){
            auto res = testStack.pop();
            if(res){
                cout<<"THREAD: "<<id<<" res="<<res->data()<<endl;
            }
        }
    };

    thread t1(producer);
    thread t2(consumer, 1);
    thread t3(consumer, 2);
    thread t4(consumer, 3);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    return 0;
}