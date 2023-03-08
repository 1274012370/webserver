/*
 * @Author       : Jie
 * @Date         : 2023-02-22
 */ 


#include"httpConn.h"
#include<iostream>

std::atomic<int> httpConn::userCount;
const char* httpConn::srcDir;
bool httpConn::isET;

httpConn::httpConn()
{
    m_fd = -1;
    addr = { 0 };
    stop = true;
}

httpConn::~httpConn()
{
    close_conn(); 
}

void httpConn::init(int sockFd, const sockaddr_in& addr)
{
    assert(sockFd > 0);
    this->addr = addr;
    m_fd = sockFd;
    stop = false;
    userCount++;

    writeBuff.retrieveAll();
    readBuff.retrieveAll();
    LOG_INFO("Client[%d](%s:%d) in, userCount:%d", m_fd, getIP(), getPort(), (int)userCount);
}

void httpConn::close_conn() 
{
    response.unmapFile();
    if(stop == false)
    {
        stop = true; 
        userCount--;
        close(m_fd);
        LOG_INFO("Client[%d](%s:%d) quit, UserCount:%d", m_fd, getIP(), getPort(), (int)userCount);
    }

}

// 循环读取客户数据，直到无数据可读或者对方关闭连接
ssize_t httpConn::read(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = readBuff.readFd(m_fd, saveErrno);
        if (len <= 0) {
            break;
        }
    }while(isET);
    
    return len;
}

ssize_t httpConn::write(int* saveErrno) {
    ssize_t len = -1;
    do {
        len = writev(m_fd, iov, iovCnt);
        if(len <= 0) {
            *saveErrno = errno;
            break;
        }
        if(iov[0].iov_len + iov[1].iov_len  == 0) { break; } /* 传输结束 */
        else if(static_cast<size_t>(len) > iov[0].iov_len) {
            iov[1].iov_base = (uint8_t*) iov[1].iov_base + (len - iov[0].iov_len);
            iov[1].iov_len -= (len - iov[0].iov_len);
            if(iov[0].iov_len) {
                writeBuff.retrieveAll();
                iov[0].iov_len = 0;
            }
        }
        else {
            iov[0].iov_base = (uint8_t*)iov[0].iov_base + len; 
            iov[0].iov_len -= len; 
            writeBuff.retrieve(len);
        }
    } while(isET || toWriteBytes() > 10240);
    return len;
}

bool httpConn::process()
{
    // 解析HTTP请求
    request.init();
    if(readBuff.readableBytes() <= 0) {
        return false;
    }
    else if(request.parse(readBuff)) {
        LOG_DEBUG("%s", request.Path().c_str());
        response.init(srcDir, request.Path(), request.isKeepAlive(), 200);
    } else {
        response.init(srcDir, request.Path(), false, 400);
    }

    response.makeResponse(writeBuff);
    /* 响应头 */
    iov[0].iov_base = const_cast<char*>(writeBuff.peek());
    iov[0].iov_len = writeBuff.readableBytes();
    iovCnt = 1;

    /* 文件 */
    if(response.fileLen() > 0  && response.file()) {
        iov[1].iov_base = response.file();
        iov[1].iov_len = response.fileLen();
        iovCnt = 2;
    }
    LOG_DEBUG("filesize:%d, %d  to %d", response.fileLen() , iovCnt, toWriteBytes());
    return true;
}


int httpConn::getFd() const {
    return m_fd;
}

struct sockaddr_in httpConn::getAddr() const {
    return addr;
}

const char* httpConn::getIP() const {
    return inet_ntoa(addr.sin_addr);
}

int httpConn::getPort() const {
    return addr.sin_port;
}