#include "av_queue.h"
#include "packet.h"
#include <memory>
#include <iostream>
#include <condition_variable>
#include <frame.h>

template<typename T>
MyQueue<T>::MyQueue() {
    head = nullptr;
    tail = nullptr;
    lenth = 0;
}

template<typename T>
void MyQueue<T>::pop_front() {
    MyNode<T>* temp = head;
    head = head->next;
    delete temp;
    lenth--;
}
template<typename T>
T MyQueue<T>::front() {
    return head->val;
}
template<typename T>
void MyQueue<T>::push_back(const T& val) {
    MyNode<T>* node = new MyNode<T>();
    node->val = val;
    if (head == nullptr) {
        head = node;
        tail = node;
    } else {
        tail->next = node;
        tail = node;
    }
    lenth++;
}
template<typename T>
void MyQueue<T>::clear() {
    MyNode<T>* temp = head;
    while (temp != nullptr) {
        MyNode<T>* next = temp->next;
        delete temp;
        temp = next;
    }
    head = nullptr;
    tail = nullptr;
    lenth = 0;
}
template<typename T>
bool MyQueue<T>::empty() {
    return lenth == 0;
}
template<typename T>
size_t MyQueue<T>::size() {
    return lenth;
}

template<typename T>
MutexQueue<T>::MutexQueue() {
    finish = false;
    items.clear();
}
template<typename T>
MutexQueue<T>::~MutexQueue() {
    items.clear();
}
template<typename T>
void MutexQueue<T>::push(const T& vedio) {
    {
        lock_guard<mutex> lock(mtx);
        items.push_back(vedio);
    }
    cv.notify_one(); // 通知等待的线程
}
template<typename T>
void MutexQueue<T>::setFinish(bool val) {
    lock_guard<mutex> lock(mtx);
    finish = val;
}
template<typename T>
bool MutexQueue<T>::isFinished() {
    lock_guard<mutex> lock(mtx);
    return finish;
}
template<typename T>
T MutexQueue<T>::pop() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [this] { return !items.empty(); }); // 阻塞直到队列不为空
    T item = items.front();
    items.pop_front();
    return item;
}
template<typename T>
bool MutexQueue<T>::empty() {
    return items.empty();
}
template<typename T>
size_t MutexQueue<T>::size() {
    lock_guard<mutex> lock(mtx);
    return items.size();
}
template class MyNode<shared_ptr<Packet>>;
template class MyQueue<shared_ptr<Packet>>;
template class MutexQueue<shared_ptr<Packet>>;

template class MyNode<shared_ptr<Frame>>;
template class MyQueue<shared_ptr<Frame>>;
template class MutexQueue<shared_ptr<Frame>>;