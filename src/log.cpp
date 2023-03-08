/*
 * @Author       : Jie
 * @Date         : 2023-03-07
 */

#include"log.h"

Log::Log() {
    lineCount = 0;
    isAsync = false;
    writeThread = nullptr;
    deque = nullptr;
    toDay = 0;
    fp = nullptr;
}

Log::~Log() {
    if(writeThread && writeThread->joinable()) {
        while(!deque->empty()) {
            deque->flush();
        };
        deque->Close();
        writeThread->join();
    }
    if(fp) {
        std::lock_guard<std::mutex> locker(mtx);
        flush();
        fclose(fp);
    }
}

int Log::getLevel() {
    std::lock_guard<std::mutex> locker(mtx);
    return level;
}

void Log::init(int level = 1, const char* path, const char* suffix,
    int maxQueueSize) {
    isOpen = true;
    this->level = level;
    if(maxQueueSize > 0) {
        isAsync = true;
        if(!deque) {
            std::unique_ptr<BlockDeque<std::string>> newDeque(new BlockDeque<std::string>);
            deque = move(newDeque);
            
            std::unique_ptr<std::thread> NewThread(new std::thread(flushLogThread));
            writeThread = move(NewThread);
        }
    } else {
        isAsync = false;
    }

    lineCount = 0;

    time_t timer = time(nullptr);
    struct tm *sysTime = localtime(&timer);
    struct tm t = *sysTime;
    this->path = path;
    this->suffix = suffix;
    char fileName[LOG_NAME_LEN] = {0};
    snprintf(fileName, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", 
            path, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix);
    toDay = t.tm_mday;

    {
        std::lock_guard<std::mutex> locker(mtx);
        buff.retrieveAll();
        if(fp) { 
            flush();
            fclose(fp); 
        }

        fp = fopen(fileName, "a");
        if(fp == nullptr) {
            mkdir(path, 0777);
            fp = fopen(fileName, "a");
        } 
        assert(fp != nullptr);
    }
}

void Log::write(int level, const char *format, ...) {
    struct timeval now = {0, 0};
    gettimeofday(&now, nullptr);
    time_t tSec = now.tv_sec;
    struct tm *sysTime = localtime(&tSec);
    struct tm t = *sysTime;
    va_list vaList;

    /* 日志日期 日志行数 */
    if (toDay != t.tm_mday || (lineCount && (lineCount  %  MAX_LINES == 0)))
    {
        std::unique_lock<std::mutex> locker(mtx);
        locker.unlock();
        
        char newFile[LOG_NAME_LEN];
        char tail[36] = {0};
        snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

        if (toDay != t.tm_mday)
        {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s%s", path, tail, suffix);
            toDay = t.tm_mday;
            lineCount = 0;
        }
        else {
            snprintf(newFile, LOG_NAME_LEN - 72, "%s/%s-%d%s", path, tail, (lineCount  / MAX_LINES), suffix);
        }
        
        locker.lock();
        flush();
        fclose(fp);
        fp = fopen(newFile, "a");
        assert(fp != nullptr);
    }

    {
        std::unique_lock<std::mutex> locker(mtx);
        lineCount++;
        int n = snprintf(buff.beginWrite(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ",
                    t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                    t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
                    
        buff.hasWritten(n);
        appendLogLevelTitle(level);

        va_start(vaList, format);
        int m = vsnprintf(buff.beginWrite(), buff.writableBytes(), format, vaList);
        va_end(vaList);

        buff.hasWritten(m);
        buff.append("\n\0", 2);

        if(isAsync && deque && !deque->full()) {
            deque->push_back(buff.retrieveAllToStr());
        } else {
            fputs(buff.peek(), fp);
        }
        buff.retrieveAll();
    }
}

void Log::appendLogLevelTitle(int level) {
    switch(level) {
    case 0:
        buff.append("[debug]: ", 9);
        break;
    case 1:
        buff.append("[info] : ", 9);
        break;
    case 2:
        buff.append("[warn] : ", 9);
        break;
    case 3:
        buff.append("[error]: ", 9);
        break;
    default:
        buff.append("[info] : ", 9);
        break;
    }
}

void Log::flush() {
    if(isAsync) { 
        deque->flush(); 
    }
    fflush(fp);
}

void Log::asyncWriteLog() {
    std::string str = "";
    while(deque->pop(str)) {
        std::lock_guard<std::mutex> locker(mtx);
        fputs(str.c_str(), fp);
    }
}

Log* Log::getInstance() {
    static Log inst;
    return &inst;
}

void Log::flushLogThread() {
    Log::getInstance()->asyncWriteLog();
}