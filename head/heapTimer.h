/*
 * @Author       : Jie
 * @Date         : 2023-03-07
 */ 

#ifndef HEAPTIMER_H_
#define HEAPTIMER_H_

#include <queue>
#include <unordered_map>
#include <time.h>
#include <algorithm>
#include <arpa/inet.h> 
#include <functional> 
#include <assert.h> 
#include <chrono>

using TimeoutCallBack = std::function<void()>;
using Clock = std::chrono::high_resolution_clock;   // ns级时钟
using MS = std::chrono::milliseconds;
using TimeStamp = Clock::time_point;      // 具体时间

struct timerNode {
    int id;
    TimeStamp expires;
    TimeoutCallBack cb;
    bool operator<(const timerNode& t) {
        return expires < t.expires;
    }
};

class heapTimer {
public:
    heapTimer() { heap.reserve(64); }

    ~heapTimer() { clear(); }
    
    void adjust(int id, int newExpires);

    void add(int id, int timeOut, const TimeoutCallBack& cb);

    void doWork(int id);

    void clear();

    void tick();

    void pop();

    int getNextTick();

private:
    void del(size_t i);
    
    void siftup(size_t i);

    bool siftdown(size_t index, size_t n);

    void swapNode(size_t i, size_t j);

    std::vector<timerNode> heap;

    std::unordered_map<int, size_t> ref;
};

#endif