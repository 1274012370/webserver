/*
 * @Author       : Jie
 * @Date         : 2023-02-19
 */ 

#ifndef __EPOLLER__
#define __EPOLLER__

#include<sys/epoll.h> //epoll_ctl()
#include<fcntl.h>  // fcntl()
#include<unistd.h> // close()
#include<assert.h> // close()
#include<vector>
#include<errno.h>


// I/O多路复用，同时监听多个文件描述符,提高程序的性能
/*
在内核中创建了一个数据，一个是需要检测的文件描述符的信息（红黑树），
还有一个是就绪列表，存放检测到数据发送改变的文件描述符信息（双向链表）。
*/
class Epoller 
{
    public:
        Epoller(int maxEvent = 1024);
        ~Epoller();

        bool addFd(int fd, u_int32_t events);   //添加监听的文件描述符
        bool modFd(int fd, u_int32_t events);   //修改
        bool delFd(int fd);                     //删除
        int wait(int timeout);                  //检测fd数据发生变化

        int getEventFd(size_t i) const;
        u_int32_t getEvents(size_t i) const;

    private:
        int epollFd;
        /*
            struct epoll_event {
                uint32_t 		events; // Epoll events
                epoll_data_t 	data; // User data variable
            };

            常见的Epoll检测事件：
                        - EPOLLIN - EPOLLOUT - EPOLLERR
        */
        std::vector<struct epoll_event> events;     //存放发生变化的文件描述符
};


#endif