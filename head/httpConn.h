/*
 * @Author       : Jie
 * @Date         : 2023-02-22
 */ 

#ifndef __HTTPCONN__
#define __HTTPCONN__

#include<sys/stat.h>
#include<sys/types.h>
#include<sys/uio.h>     // readv/writev
#include<arpa/inet.h>   // sockaddr_in
#include<stdlib.h>      // atoi()
#include<errno.h>  
#include<atomic>
#include"httpRequest.h"
#include"httpResponse.h"
#include"buffer.h"
#include"epoller.h"
#include"log.h"

class httpConn
{
    public:
        httpConn();
        ~httpConn();
        void init(int sockFd, const sockaddr_in& addr); // 初始化新接受的连接
        ssize_t read(int* saveErrno);
        ssize_t write(int* saveErrno);
        void close_conn();                               // 关闭连接
        bool process();                                 //处理客户端的请求

        int toWriteBytes() { 
            return iov[0].iov_len + iov[1].iov_len; 
        }

        bool isKeepAlive() const {
            return request.isKeepAlive();
        }

        int getFd() const;
        int getPort() const;
        const char* getIP() const;
        sockaddr_in getAddr() const;

        static bool isET;
        static const char* srcDir;
        static std::atomic<int> userCount;

    private:
        int m_fd;             //该HTTP连接的socket和对方的socket地址
        struct sockaddr_in addr;         //socket地址
        bool stop;


        int iovCnt;
        struct iovec iov[2];

        Buffer readBuff;        //读缓冲区
        Buffer writeBuff;       //写缓冲区
        httpRequest request;
        httpResponse response;
};



#endif