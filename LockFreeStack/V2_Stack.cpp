#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <memory>

using namespace std;

// Проблема состоит в том, что нельзя удалять эл-ты пока какой то другой поток
// Имеет указатель на эти элементы, поэтому мы будем помещать удаляемые элементы в некий список
// "to be deleted". Удалить этот список можно целиком, когда нет ни одного потока, который находится в функции pop()

namespace v2 {
    template<typename T>
    struct node {
        node(T data) : data(make_shared<T>(data)), next(nullptr) {}

        shared_ptr<T> data;
        node *next;
    };

    template<typename T>
    struct Stack {
        using node = node<T>;
        atomic<node *> head{nullptr};
        atomic<int> threadsInPop{0};

        void push(T data) {
            auto newNode = new node(data);
            newNode->next = head.load();
            while (!head.compare_exchange_weak(newNode->next, newNode));
        }

        shared_ptr<T> pop() {
            ++threadsInPop;
            auto *oldHead = head.load();
            // меняем голову
            while (oldHead && !head.compare_exchange_weak(oldHead, oldHead->next));
            std::shared_ptr<T> res{nullptr};
            if (oldHead) {
                res.swap(oldHead->data);
            }
            try_reclaim(oldHead);
            return res;
        }

        // Код удаления
        // Голова списка удаления
        atomic<node *> to_be_deleted{nullptr};

        static void delete_nodes(node *nodes) {
            while (nodes) {
                node *next = nodes->next;
                delete nodes;
                nodes = next;
            }
        }

        void try_reclaim(node *old_head) {
            if (threadsInPop == 1) { // в данный момент мы единственный поток в pop();                (1)
                // Забираем текущий список себе
                node *nodes_to_delete = to_be_deleted.exchange(nullptr); //                        (2)
                if (!--threadsInPop) { // от первой проверки до этой еще один поток мог зайти в pop() (3)
                    // если никто не зашел спокойно удаляем
                    // Список удаления целиком наш
                    delete_nodes(nodes_to_delete);//                                                  (4)
                } else if (nodes_to_delete) {//                                                       (5)
                    // Если еще кто то пришел в pop(), надо вернуть список на место
                    chain_pending_nodes(nodes_to_delete);//                                           (6)
                }
                // Голову можно спокойно удалять, новый поток УЖЕ получит другое значение головы
                delete old_head; //                                                                   (7)
            } else {
                chain_pending_node(old_head);//                                                       (8)
                --threadsInPop;
            }
        }

        void chain_pending_nodes(node *nodes) {
            node *last = nodes;
            while (node *const next = last->next) { //                                                (9)
                last = next;
            }
            chain_pending_nodes(nodes, last);
        }

        void chain_pending_nodes(node *first, node *last) {
            last->next = to_be_deleted;//                                                             (10)
            while (!to_be_deleted.compare_exchange_weak(last->next, first));//                        (11)
        }

        void chain_pending_node(node *n) {
            chain_pending_nodes(n, n); //                                                             (12)
        }

    };


}

int main() {
    v2::Stack<string> testStack;
    auto producer = [&testStack]() {
        string prefix = "name_";
        for (int i = 0; i < 1000; ++i)
            testStack.push(prefix + to_string(i));
    };

    auto consumer = [&testStack](const int id) {
        for (int i = 0; i < 1000; ++i) {
            auto res = testStack.pop();
            if (res) {
                cout << "THREAD: " << id << " res=" << res->data() << endl;
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