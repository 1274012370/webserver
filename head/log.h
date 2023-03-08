/*
 * @Author       : Jie
 * @Date         : 2023-03-07
 */

#ifndef LOG_H_
#define LOG_H_

#include <mutex>
#include <string>
#include <thread>
#include <sys/time.h>
#include <string.h>
#include <stdarg.h>           // vastart va_end
#include <assert.h>
#include <sys/stat.h>         //mkdir
#include "blockqueue.h"
#include "buffer.h"

class Log {
    public:
        // 初始化日志
        void init(int level, const char* path = "./log", 
                    const char* suffix =".log",
                    int maxQueueCapacity = 1024);

        // 懒汉式单例模式
        // C++11编译器保证函数内的局部静态对象的线程安全性
        static Log* getInstance();
        // 异步写入方式公有方法
        static void flushLogThread();
        // 向日志文件写入具体内容
        void write(int level, const char *format,...);
        // 强制刷新缓冲区
        void flush();

        int getLevel();
        bool IsOpen() { return isOpen; }
    private:
        Log();
        void appendLogLevelTitle(int level);
        virtual ~Log();
        void asyncWriteLog();


        static const int LOG_PATH_LEN = 256;
        static const int LOG_NAME_LEN = 256;
        static const int MAX_LINES = 50000;

        const char* path;
        const char* suffix;

        int max_lines;

        int lineCount;
        int toDay;

        bool isOpen;
    
        Buffer buff;
        int level;
        bool isAsync;

        FILE* fp;
        std::unique_ptr<BlockDeque<std::string>> deque; 
        std::unique_ptr<std::thread> writeThread;
        std::mutex mtx;
};

#define LOG_BASE(level, format, ...) \
    do {\
        Log* log = Log::getInstance();\
        if (log->IsOpen() && log->getLevel() <= level) {\
            log->write(level, format, ##__VA_ARGS__); \
            log->flush();\
        }\
    } while(0);

#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif