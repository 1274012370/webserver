/*
 * @Author       : Jie
 * @Date         : 2023-02-13
 */ 

#ifndef __THREADPOOL__
#define __THREADPOOL__

#include<mutex>
#include<condition_variable>
#include<queue>
#include<vector>
#include<thread>
#include<memory>
#include<iostream>
#include<functional>

const int MAX_THREADS = 1000;    //最大线程数目

//线程池类，定义成模板类
class threadPool 
{
		using Task = std::function<void(void)>;

    public:
        explicit threadPool(int thread_number = 8);
        ~threadPool();

		threadPool() = default;
		threadPool(threadPool&&) = default;

        //向请求队列中插入任务请求
        bool append(Task task);

    private:
        //工作线程运行的函数,静态函数无法访问内部成员
        static void* worker(void* arg);
        //run执行任务
        void run();

    private:
        std::vector<std::thread> work_threads;  //线程池
        std::queue<Task> tasks_queue;             //任务队列
        std::mutex queue_mutex;                 //保护任务队列的互斥锁
        std::condition_variable cond;           //是否有任务需要处理
        bool stop;                              //是否结束线程

};


#endif