/*
 * @Author       : Jie
 * @Date         : 2023-02-23
 */ 
#ifndef __HTTPREQUEST__
#define __HTTPREQUEST__

#include"buffer.h"
#include<string>
#include <regex>
#include<unordered_map>
#include"log.h"

class httpRequest
{

    public:

        /*
            服务器处理HTTP请求的可能结果，报文解析的结果
            NO_REQUEST          :   请求不完整，需要继续读取客户数据
            GET_REQUEST         :   表示获得了一个完成的客户请求
            BAD_REQUEST         :   表示客户请求语法错误
            NO_RESOURCE         :   表示服务器没有资源
            FORBIDDEN_REQUEST   :   表示客户对资源没有足够的访问权限
            FILE_REQUEST        :   文件请求,获取文件成功
            INTERNAL_ERROR      :   表示服务器内部错误
            CLOSED_CONNECTION   :   表示客户端已经关闭连接了
        */
        enum HTTP_CODE { 
            NO_REQUEST, 
            GET_REQUEST, 
            BAD_REQUEST, 
            NO_RESOURCE, 
            FORBIDDEN_REQUEST, 
            FILE_REQUEST, 
            INTERNAL_ERROR, 
            CLOSED_CONNECTION 
        };

        /*
            解析客户端请求时，主状态机的状态
            REQUEST_LINE:当前正在分析请求行
            HEADER:当前正在分析头部字段
            BODY:当前正在解析请求体
        */
        enum PARSE_STATE { 
            REQUEST_LINE = 0, 
            HEADER, 
            BODY,
            FINISH,
        };
        

        httpRequest() { init(); };
        ~httpRequest() = default;

        void init();
        bool parse(Buffer& buff);    // 解析HTTP请求

        // 下面这一组函数被process_read调用以分析HTTP请求
        void parsePath();
        bool parseRequestLine(const std::string& line);
        void parseHeader(const std::string& line);
        void parseBody(const std::string& line);

        std::string& Path() { return path; };
        bool isKeepAlive() const;
        void print();
    private:
        std::string method, path, version, body;
        std::unordered_map<std::string, std::string> header;
        PARSE_STATE state;          //当前主状态机的状态



};





#endif