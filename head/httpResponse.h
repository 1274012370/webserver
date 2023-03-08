/*
 * @Author       : Jie
 * @Date         : 2023-02-23
 */ 

#ifndef __HTTPRESPONSE__
#define __HTTPRESPONSE__

#include <unordered_map>
#include <fcntl.h>       // open
#include <unistd.h>      // close
#include <sys/stat.h>    // stat
#include <sys/mman.h>    // mmap, munmap

#include"buffer.h"
#include"log.h"

class httpResponse
{
    public:
        httpResponse();
        ~httpResponse();

        void init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code = -1);
        void makeResponse(Buffer& buff);

        void unmapFile();
        char* file();
        size_t fileLen() const;

        void errorContent(Buffer& buff, std::string message);
        int getCode() const { return code; }
    private:
        void addStateLine(Buffer &buff);
        void addHeader(Buffer &buff);
        void addContent(Buffer &buff);

        std::string getFileType();

        int code;
        bool isKeepAlive;

        std::string path;
        std::string srcDir;

        char* mmFile; 
        struct stat mmFileStat;

        static const std::unordered_map<std::string, std::string> SUFFIX_TYPE;
        static const std::unordered_map<int, std::string> CODE_STATUS;
};







#endif