#pragma once
#include <mutex>
#include <condition_variable>
using namespace std;
template<typename T>
class RingBuffer 
{
private:
    T* buffer;
    int head;
    int tail;
    int count;
    int maxSize;
    mutex mtx;
    condition_variable cv;
    bool isFinish;
    
public:
    RingBuffer(int size);
    ~RingBuffer();
    void push(const T& val);
    T pop();
    bool empty();
    bool full();
    int size();
    void setFinish(bool finish);
    bool isFinished();
};