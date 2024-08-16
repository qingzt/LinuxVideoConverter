#pragma once
extern "C" {
#include <libavformat/avformat.h>
}
#include <list>
#include <mutex>
#include <condition_variable>
using namespace std;


template<typename T>
class MyNode{ //队列节点
public:
    T val;
    MyNode<T>* next;
    MyNode<T>(){
        next=nullptr;
    }
};

template<typename T>
class MyQueue{ //队列
private:
    MyNode<T>* head;
    MyNode<T>* tail;
    size_t lenth;
public:
    MyQueue();
    T front();
    void pop_front();
    void push_back(const T& val);
    bool empty();
    size_t size();
    void clear();
};

template<typename T>
class MutexQueue { //线程安全队列
private:
    MyQueue<T> items;
    mutex mtx;                      // 互斥锁，保证线程安全
    condition_variable cv;          // 条件变量，用于线程同步
    bool finish;                    // 标志位，表示队列是否已经结束
public:
    MutexQueue();
    ~MutexQueue();
    void push(const T& vedio);// 入队
    T pop();                 // 出队
    bool empty();                   // 判断队列是否为空
    size_t size();                  // 获取队列大小
    void setFinish(bool val);       // 设置结束标志
    bool isFinished();              // 判断队列是否结束
};