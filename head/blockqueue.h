/*
 * @Author       : Jie
 * @Date         : 2023-03-07
 */ 

#ifndef BLOCKQUEUE_H_
#define BLOCKQUEUE_H_

#include <mutex>
#include <deque>
#include <condition_variable>
#include <sys/time.h>

template<class T>
class BlockDeque {
public:
    explicit BlockDeque(size_t MaxCapacity = 1000);
    ~BlockDeque();

    void clear();
    bool empty();
    bool full();
    void Close();
    size_t size();
    size_t getCapacity();

    T front();
    T back();

    void push_back(const T &item);
    void push_front(const T &item);
    bool pop(T &item);
    bool pop(T &item, int timeout);

    void flush();

private:
    std::deque<T> deq;
    size_t capacity;
    std::mutex mtx;
    bool isClose;

    std::condition_variable condConsumer;
    std::condition_variable condProducer;
};


template<class T>
BlockDeque<T>::BlockDeque(size_t MaxCapacity):capacity(MaxCapacity) {
    assert(MaxCapacity > 0);
    isClose = false;
}

template<class T>
BlockDeque<T>::~BlockDeque() {
    Close();
};

template<class T>
void BlockDeque<T>::Close() {
    {   
        std::lock_guard<std::mutex> locker(mtx);
        deq.clear();
        isClose = true;
    }
    condProducer.notify_all();
    condConsumer.notify_all();
};

template<class T>
void BlockDeque<T>::flush() {
    condConsumer.notify_one();
};

template<class T>
void BlockDeque<T>::clear() {
    std::lock_guard<std::mutex> locker(mtx);
    deq.clear();
}

template<class T>
T BlockDeque<T>::front() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.front();
}

template<class T>
T BlockDeque<T>::back() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.back();
}

template<class T>
size_t BlockDeque<T>::size() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.size();
}

template<class T>
size_t BlockDeque<T>::getCapacity() {
    std::lock_guard<std::mutex> locker(mtx);
    return capacity;
}

template<class T>
void BlockDeque<T>::push_back(const T &item) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.size() >= capacity) {
        condProducer.wait(locker);
    }
    deq.push_back(item);
    condConsumer.notify_one();
}

template<class T>
void BlockDeque<T>::push_front(const T &item) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.size() >= capacity) {
        condProducer.wait(locker);
    }
    deq.push_front(item);
    condConsumer.notify_one();
}

template<class T>
bool BlockDeque<T>::empty() {
    std::lock_guard<std::mutex> locker(mtx);
    return deq.empty();
}

template<class T>
bool BlockDeque<T>::full(){
    std::lock_guard<std::mutex> locker(mtx);
    return deq.size() >= capacity;
}

template<class T>
bool BlockDeque<T>::pop(T &item) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.empty()){
        condConsumer.wait(locker);
        if(isClose){
            return false;
        }
    }
    item = deq.front();
    deq.pop_front();
    condProducer.notify_one();
    return true;
}

template<class T>
bool BlockDeque<T>::pop(T &item, int timeout) {
    std::unique_lock<std::mutex> locker(mtx);
    while(deq.empty()){
        if(condConsumer.wait_for(locker, std::chrono::seconds(timeout)) 
                == std::cv_status::timeout){
            return false;
        }
        if(isClose){
            return false;
        }
    }
    item = deq.front();
    deq.pop_front();
    condProducer.notify_one();
    return true;
}

#endif