/*
 * @Author       : Jie
 * @Date         : 2023-02-23
 */ 

#ifndef __BUFFER__
#define __BUFFER__

#include <cstring>   //perror
#include <iostream>
#include <unistd.h>  // write
#include <sys/uio.h> //readv
#include <vector> //readv
#include <atomic>
#include <assert.h>
#include<stddef.h>

class Buffer 
{
    public:
        Buffer(int size = 1024);
        ~Buffer() = default;

        size_t writableBytes() const;       //可读取的字节数
        size_t readableBytes() const;       //可写的字节数
        size_t prependableBytes() const;    //可预先准备的字节数

        const char* peek() const;           //可读的头地址
        void ensureWriteable(size_t len);   //确保缓冲区可写
        void hasWritten(size_t len);        //写指针向后移len位

        void retrieve(size_t len);              //使读指针加len个长度
        void retrieveUntil(const char* end);    //使读指针加end-peek()个长度

        void retrieveAll() ;                    //恢复所有空间
        std::string retrieveAllToStr();         //读取所有字符返回字符串，并恢复所有空间

        const char* beginWriteConst() const;    //返回可写的首地址
        char* beginWrite();                     //返回可写的首地址

        void append(const std::string& str);
        void append(const char* str, size_t len);
        void append(const void* data, size_t len);
        void append(const Buffer& buff);

        ssize_t readFd(int fd, int* Errno);
        ssize_t writeFd(int fd, int* Errno);

        void print() const;

    private:
        char* beginPtr();                   //缓冲区开始指针
        const char* beginPtr() const;       //缓冲区开始指针
        void makeSpace(size_t len);         //动态扩展缓冲区

        std::vector<char> buffer;
        std::atomic<std::size_t> readPos;   //可以读取数据的开始位置
        std::atomic<std::size_t> writePos;  //可以写入数据的起始位置
};



#endif