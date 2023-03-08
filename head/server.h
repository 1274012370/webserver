/*
 * @Author       : Jie
 * @Date         : 2023-03-06
 */ 


#ifndef __SERVER__
#define __SERVER__

#include <unordered_map>
#include <fcntl.h>       // fcntl()
#include <unistd.h>      // close()
#include <assert.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<signal.h>

#include "epoller.h"
#include "threadPool.h"
#include "httpConn.h"
#include"heapTimer.h"
#include"log.h"

class Server
{
    public:
        Server(int port, int threadNum, int trigMode, int timeoutMS, 
                bool openLog, int logLevel, int logQueSize);
        ~Server();
        void start();
    private:
        bool initSocket();
        void initEventMode(int trigMode);

        void dealListen();
        void dealWrite(httpConn* client);
        void dealRead(httpConn* client);

        void closeConn(httpConn* client);

        void onRead(httpConn* client);
        void onWrite(httpConn* client);
        void onProcess(httpConn* client);

        void addClient(int fd, sockaddr_in addr);
        void extentTime(httpConn* client);
        
        static int setFdNonblock(int fd);
        static const int MAX_FD = 65536;

        int port;
        bool stop;
        int listenFd;
        int timeoutMS;  /* 毫秒MS */

        u_int32_t listenEvent;
        u_int32_t connEvent;

        threadPool *pool;
        Epoller *epoller;
        std::unordered_map<int, httpConn> users;
        std::unique_ptr<heapTimer> timer;

};

#endif