/*
 * @Author       : Jie
 * @Date         : 2023-03-07
 */ 

#include"heapTimer.h"

void heapTimer::siftup(size_t i) {
    assert(i >= 0 && i < heap.size());
    size_t j = (i - 1) / 2;
    while(j >= 0) {
        if(heap[j] < heap[i]) { break; }
        swapNode(i, j);
        i = j;
        j = (i - 1) / 2;
    }
}

void heapTimer::swapNode(size_t i, size_t j) {
    assert(i >= 0 && i < heap.size());
    assert(j >= 0 && j < heap.size());
    std::swap(heap[i], heap[j]);
    ref[heap[i].id] = i;
    ref[heap[j].id] = j;
} 

bool heapTimer::siftdown(size_t index, size_t n) {
    assert(index >= 0 && index < heap.size());
    assert(n >= 0 && n <= heap.size());
    size_t i = index;
    size_t j = i * 2 + 1;
    while(j < n) {
        if(j + 1 < n && heap[j + 1] < heap[j]) j++;
        if(heap[i] < heap[j]) break;
        swapNode(i, j);
        i = j;
        j = i * 2 + 1;
    }
    return i > index;
}

void heapTimer::add(int id, int timeout, const TimeoutCallBack& cb) {
    assert(id >= 0);
    size_t i;
    if(ref.count(id) == 0) {
        /* 新节点：堆尾插入，调整堆 */
        i = heap.size();
        ref[id] = i;
        heap.push_back({id, Clock::now() + MS(timeout), cb});
        siftup(i);
    } 
    else {
        /* 已有结点：调整堆 */
        i = ref[id];
        heap[i].expires = Clock::now() + MS(timeout);
        heap[i].cb = cb;
        if(!siftdown(i, heap.size())) {
            siftup(i);
        }
    }
}

void heapTimer::doWork(int id) {
    /* 删除指定id结点，并触发回调函数 */
    if(heap.empty() || ref.count(id) == 0) {
        return;
    }
    size_t i = ref[id];
    timerNode node = heap[i];
    node.cb();
    del(i);
}

void heapTimer::del(size_t index) {
    /* 删除指定位置的结点 */
    assert(!heap.empty() && index >= 0 && index < heap.size());
    /* 将要删除的结点换到队尾，然后调整堆 */
    size_t i = index;
    size_t n = heap.size() - 1;
    assert(i <= n);
    if(i < n) {
        swapNode(i, n);
        if(!siftdown(i, n)) {
            siftup(i);
        }
    }
    /* 队尾元素删除 */
    ref.erase(heap.back().id);
    heap.pop_back();
}

void heapTimer::adjust(int id, int timeout) {
    /* 调整指定id的结点 */
    assert(!heap.empty() && ref.count(id) > 0);
    heap[ref[id]].expires = Clock::now() + MS(timeout);;
    siftdown(ref[id], heap.size());
}

void heapTimer::tick() {
    /* 清除超时结点 */
    if(heap.empty()) {
        return;
    }
    while(!heap.empty()) {
        timerNode node = heap.front();
        if(std::chrono::duration_cast<MS>(node.expires - Clock::now()).count() > 0) { 
            break; 
        }
        node.cb();
        pop();
    }
}

void heapTimer::pop() {
    assert(!heap.empty());
    del(0);
}

void heapTimer::clear() {
    ref.clear();
    heap.clear();
}

int heapTimer::getNextTick() {
    tick();
    size_t res = -1;
    if(!heap.empty()) {
        res = std::chrono::duration_cast<MS>(heap.front().expires - Clock::now()).count();
        if(res < 0) { res = 0; }
    }
    return res;
}