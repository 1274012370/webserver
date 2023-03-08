/*
 * @Author       : Jie
 * @Date         : 2023-02-13
 */ 

#include"threadPool.h"

threadPool::threadPool(int thread_number) : stop(false)
{
	if (thread_number <= 0 || thread_number > MAX_THREADS)
		throw std::exception();
	for (int i = 0; i < thread_number; i++)
	{
		work_threads.emplace_back(worker, this);
		std::cout << "创建第" << i << "个线程: " <<work_threads[i].get_id()<< std::endl;
		work_threads[i].detach();
	}
	std::cout<<"=======================================\n";
}

inline threadPool::~threadPool()
{
	{
		std::unique_lock<std::mutex> lock(queue_mutex);
		stop = true;
	}
	cond.notify_all();
		
}

bool threadPool::append(Task task)
{
	/*操作工作队列时一定要加锁，因为他被所有线程共享*/
	queue_mutex.lock();
	tasks_queue.push(task);
	queue_mutex.unlock();
	cond.notify_one(); //线程池添加进去了任务，自然要通知等待的线程
	return true;
}

void *threadPool::worker(void *arg)
{
	threadPool *pool = (threadPool *)arg;
	pool->run();
	return pool;
}

void threadPool::run()
{
	while (!stop)
	{
		std::unique_lock<std::mutex> lk(this->queue_mutex);
		/*　unique_lock() 出作用域会自动解锁　*/
		this->cond.wait(lk, [this] { return !this->tasks_queue.empty(); });
		//如果任务队列不为空，就停下来等待唤醒
		if (this->tasks_queue.empty())
		{
			continue;
		}
		else
		{
			Task task = tasks_queue.front();
			tasks_queue.pop();
            lk.unlock();
			if(task) task();
				
		}
	}
}