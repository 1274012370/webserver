/*
 * @Author       : Jie
 * @Date         : 2023-02-23
 */ 

#include"httpRequest.h"

void httpRequest::init()
{
    state = REQUEST_LINE;   //初始状态为解析请求首行
}

// 主状态机，解析请求
bool httpRequest::parse(Buffer& buff) {

    const char CRLF[] = "\r\n";
    if(buff.readableBytes() <= 0)return false;

    while (buff.readableBytes() && state != FINISH)
    {
        const char* lineEnd = std::search(buff.peek(), buff.beginWriteConst(), CRLF, CRLF + 2);
        std::string line(buff.peek(), lineEnd);

        switch(state)
        {
            case REQUEST_LINE:
                if(!parseRequestLine(line)) {
                    return false;
                }
                parsePath();
                break;
            case HEADER:
                parseHeader(line);
                if(buff.readableBytes() <= 2) {
                    state = FINISH;
                }
                break;
            case BODY:
                parseBody(line);
                break;
            default:
                break;
        }

        if(lineEnd == buff.beginWrite()) { break; }
        buff.retrieveUntil(lineEnd + 2);

    }
    LOG_DEBUG("[%s], [%s], [%s]", method.c_str(), path.c_str(), version.c_str());
    return true;
}

void httpRequest::parsePath() {
    if(path == "/") {
        path = "/index.html"; 
    }
}

bool httpRequest::parseRequestLine(const std::string& line)
{
    std::regex patten("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {   
        method = subMatch[1];
        path = subMatch[2];
        version = subMatch[3];
        state = HEADER;
        return true;
    }
    LOG_ERROR("RequestLine Error");
    return false;
}

void httpRequest::parseHeader(const std::string& line) {
    std::regex patten("^([^:]*): ?(.*)$");
    std::smatch subMatch;
    if(regex_match(line, subMatch, patten)) {
        header[subMatch[1]] = subMatch[2];
    }
    else {
        state = BODY;
    }
}

void httpRequest::parseBody(const std::string& line) {
    body = line;
    state = FINISH;
    LOG_DEBUG("Body:%s, len:%d", line.c_str(), line.size());
}

bool httpRequest::isKeepAlive() const {
    if(header.count("Connection") == 1) {
        return header.find("Connection")->second == "keep-alive" && version == "1.1";
    }
    return false;
}


void httpRequest::print() {
    std::cout<<"请求方法："<<method<<" 请求路径："<<path<<" 版本："<<version<<std::endl;
    std::cout<<"请求头：\n";
    for(auto i:header){
        std::cout<<i.first<<":"<<i.second<<std::endl;
    }
    std::cout<<"请求体："<<body<<std::endl;
    std::cout<<"=====================================================\n";
}