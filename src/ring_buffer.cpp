#include "ring_buffer.h"
#include <frame.h>
using namespace std;
template<typename T>
RingBuffer<T>::RingBuffer(int size)
{
    head = 0;
    tail = 0;
    count = 0;
    this->maxSize = size;
    buffer = new T[size];
    isFinish = false;
}

template<typename T>
RingBuffer<T>::~RingBuffer()
{
    delete[] buffer;
}


template<typename T>
void RingBuffer<T>::push(const T& val)
{
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [this] { return !this->full(); });
    buffer[tail] = val;
    tail = (tail + 1) % maxSize;
    count++;
    cv.notify_all();
}

template<typename T>
T RingBuffer<T>::pop()
{
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [this] { return !this->empty(); });
    T val = buffer[head];
    head = (head + 1) % maxSize;
    count--;
    cv.notify_all();
    return val;
}

template<typename T>
bool RingBuffer<T>::empty()
{
    return count == 0;
}

template<typename T>
bool RingBuffer<T>::full()
{
    return count == maxSize;
}

template<typename T>
int RingBuffer<T>::size()
{
    lock_guard<mutex> lock(mtx);
    return count;
}

template<typename T>
void RingBuffer<T>::setFinish(bool finish)
{
    lock_guard<mutex> lock(mtx);
    isFinish = finish;
}

template<typename T>
bool RingBuffer<T>::isFinished()
{
    lock_guard<mutex> lock(mtx);
    return isFinish;
}

template class RingBuffer<shared_ptr<Frame>>;