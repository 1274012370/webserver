/*
 * @Author       : Jie
 * @Date         : 2023-03-06
 */ 

#include"server.h"

Server::Server(int port, int threadNum, int trigMode, int timeoutMS,
                bool openLog, int logLevel, int logQueSize):
                port(port), pool(new threadPool(threadNum)), timer(new heapTimer()), 
                stop(false), timeoutMS(timeoutMS), epoller(new Epoller()) {
    httpConn::userCount = 0;
    httpConn::srcDir = "/home/lu/c++/webserver/resources";
    if(!initSocket()) { stop = true; }
    std::cout<<stop<<std::endl;
    initEventMode(trigMode);

    if(openLog) {
        Log::getInstance()->init(logLevel, "./log", ".log", logQueSize);
        if(stop) { LOG_ERROR("========== Server init error!=========="); }
        else {
            LOG_INFO("============= Server init =============");
            LOG_INFO("Port:%d, OpenLinger: %s", port);
            LOG_INFO("Listen Mode: %s, OpenConn Mode: %s",
                            (listenEvent & EPOLLET ? "ET": "LT"),
                            (connEvent & EPOLLET ? "ET": "LT"));
            LOG_INFO("LogSys level: %d", logLevel);
            LOG_INFO("srcDir: %s", httpConn::srcDir);
            LOG_INFO("ThreadPool num: %d", threadNum);
        }
    }
}

void Server::initEventMode(int trigMode) {
    listenEvent = EPOLLRDHUP;
    connEvent = EPOLLONESHOT | EPOLLRDHUP;
    switch (trigMode)
    {
    case 0:
        break;
    case 1:
        connEvent |= EPOLLET;
        break;
    case 2:
        listenEvent |= EPOLLET;
        break;
    case 3:
        listenEvent |= EPOLLET;
        connEvent |= EPOLLET;
        break;
    default:
        listenEvent |= EPOLLET;
        connEvent |= EPOLLET;
        break;
    }
    httpConn::isET = (connEvent & EPOLLET);
}

Server::~Server() {
    close(listenFd);
    stop = true;
}


void Server::start() {
    int timeMS = -1;  /* epoll wait timeout == -1 无事件将阻塞 */
    while(!stop) {
        if(timeoutMS > 0) {
            timeMS = timer->getNextTick();
        }
        int eventCnt = epoller->wait(timeMS);
        for(int i = 0; i < eventCnt; i++) {
            /* 处理事件 */
            int fd = epoller->getEventFd(i);
            uint32_t events = epoller->getEvents(i);

            if(fd == listenFd) {
                dealListen();
            }
            else if(events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                assert(users.count(fd) > 0);
                closeConn(&users[fd]);
            }
            else if(events & EPOLLIN) {
                assert(users.count(fd) > 0);
                dealRead(&users[fd]);
            }
            else if(events & EPOLLOUT) {
                assert(users.count(fd) > 0);
                dealWrite(&users[fd]);
            } else {
                LOG_ERROR("Unexpected event");
            }
        }
    }
}

void Server::closeConn(httpConn* client) {
    assert(client);
    LOG_INFO("Client[%d] quit!", client->getFd());
    epoller->delFd(client->getFd());
    client->close_conn();
}

void Server::addClient(int fd, sockaddr_in addr) {
    assert(fd > 0);
    users[fd].init(fd, addr);
    if(timeoutMS > 0) {
        timer->add(fd, timeoutMS, std::bind(&Server::closeConn, this, &users[fd]));
    }
    epoller->addFd(fd, connEvent | EPOLLIN);
    setFdNonblock(fd);
    LOG_INFO("Client[%d] in!", users[fd].getFd());
}

void Server::extentTime(httpConn* client) {
    assert(client);
    if(timeoutMS > 0) { timer->adjust(client->getFd(), timeoutMS); }
}

void Server::dealListen() {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    do{
        int fd = accept(listenFd, (struct sockaddr *)&addr, &len);
        if(fd <= 0) { return;}
        else if(httpConn::userCount >= MAX_FD) {
            close(fd);
            LOG_WARN("Clients is full!");
            return;
        }
        addClient(fd, addr);

    }while(listenEvent & EPOLLET);
    
}

    

void Server::dealRead(httpConn* client) {
    assert(client);
    extentTime(client);
    pool->append(std::bind(&Server::onRead, this, client));
}

void Server::dealWrite(httpConn* client) {
    assert(client);
    extentTime(client);
    pool->append(std::bind(&Server::onWrite, this, client));
}

void Server::onRead(httpConn* client) {
    assert(client);
    int ret = -1;
    int readErrno = 0;
    ret = client->read(&readErrno);
    if(ret <= 0 && readErrno != EAGAIN) {
        closeConn(client);
        return;
    }
    onProcess(client);
}

void Server::onProcess(httpConn* client) {
    if(client->process()) {
        epoller->modFd(client->getFd(), connEvent | EPOLLOUT);
    } else {
        epoller->modFd(client->getFd(), connEvent | EPOLLIN);
    }
}

void Server::onWrite(httpConn* client) {
    assert(client);
    int ret = -1;
    int writeErrno = 0;
    ret = client->write(&writeErrno);
    if(client->toWriteBytes() == 0) {
        /* 传输完成 */
        if(client->isKeepAlive()) {
            onProcess(client);
            return;
        }
    }
    else if(ret < 0) {
        if(writeErrno == EAGAIN) {
            /* 继续传输 */
            epoller->modFd(client->getFd(), connEvent | EPOLLOUT);
            return;
        }
    }
    closeConn(client);
}



/* Create listenFd */
bool Server::initSocket() {


    listenFd = socket(AF_INET, SOCK_STREAM, 0);
    if(listenFd < 0) {
        LOG_ERROR("Create socket error!", port);
        return false;
    }
    /* 端口复用 */
    int optval = 1;
    setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int));

    int ret;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    ret = bind(listenFd, (struct sockaddr *)&addr, sizeof(addr));
    if(ret < 0) {
        LOG_ERROR("Bind Port:%d error!", port);
        close(listenFd);
        return false;
    }

    ret = listen(listenFd, 6);

    ret = epoller->addFd(listenFd,  EPOLLIN | listenEvent);

    if(ret == 0) {
        LOG_ERROR("Add listen error!");
        close(listenFd);
        return false;
    }

    setFdNonblock(listenFd);
    LOG_INFO("Server port:%d", port);
    return true;
}

int Server::setFdNonblock(int fd) {
    assert(fd > 0);
    return fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK);
}