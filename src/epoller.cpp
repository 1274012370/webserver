/*
 * @Author       : Jie
 * @Date         : 2023-02-19
 */ 

#include"epoller.h"

/*
创建一个新的epoll实例。
int epoll_create(int size);
    - 参数：
    	size : 目前没有意义了。随便写一个数，必须大于0
    - 返回值：
        -1 : 失败
        > 0 : 文件描述符，操作epoll实例的
*/
Epoller::Epoller(int maxEvent):epollFd(epoll_create(512)), events(maxEvent)
{
    assert(epollFd >= 0 && events.size() > 0);
}

Epoller::~Epoller()
{
    close(epollFd);
}

/*
 // 对epoll实例进行管理：添加文件描述符信息，删除信息，修改信息
int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    - 参数：
        - epfd : epoll实例对应的文件描述符
        - op : 要进行什么操作
            EPOLL_CTL_ADD: 添加
            EPOLL_CTL_MOD: 修改
            EPOLL_CTL_DEL: 删除
        - fd : 要检测的文件描述符
        - event : 检测文件描述符什么事情
*/
bool Epoller::addFd(int fd, u_int32_t events) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd, EPOLL_CTL_ADD, fd, &ev);
}

bool Epoller::modFd(int fd, u_int32_t events) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    ev.data.fd = fd;
    ev.events = events;
    return 0 == epoll_ctl(epollFd, EPOLL_CTL_MOD, fd, &ev);
}

bool Epoller::delFd(int fd) {
    if(fd < 0) return false;
    epoll_event ev = {0};
    return 0 == epoll_ctl(epollFd, EPOLL_CTL_DEL, fd, &ev);
}

/*
// 检测函数
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
    - 参数：
        - epfd : epoll实例对应的文件描述符
        - events : 传出参数，保存了发送了变化的文件描述符的信息
        - maxevents : 第二个参数结构体数组的大小
        - timeout : 阻塞时间
            - 0 : 不阻塞
            - -1 : 阻塞，直到检测到fd数据发生变化，解除阻塞
            - > 0 : 阻塞的时长（毫秒）
    - 返回值：
        - 成功，返回发送变化的文件描述符的个数 > 0
        - 失败 -1
*/

int Epoller::wait(int timeout) {
    return epoll_wait(epollFd, &events[0], events.size(), timeout);
}

int Epoller::getEventFd(size_t i) const {
    assert(i < events.size() && i >= 0);
    return events[i].data.fd;
}

uint32_t Epoller::getEvents(size_t i) const {
    assert(i < events.size() && i >= 0);
    return events[i].events;
}