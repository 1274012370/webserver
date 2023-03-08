/*
 * @Author       : Jie
 * @Date         : 2023-02-23
 */ 

#include"httpResponse.h"

const std::unordered_map<std::string, std::string> httpResponse::SUFFIX_TYPE = {
    { ".html",  "text/html" },
    { ".xml",   "text/xml" },
    { ".xhtml", "application/xhtml+xml" },
    { ".txt",   "text/plain" },
    { ".rtf",   "application/rtf" },
    { ".pdf",   "application/pdf" },
    { ".word",  "application/nsword" },
    { ".png",   "image/png" },
    { ".gif",   "image/gif" },
    { ".jpg",   "image/jpeg" },
    { ".jpeg",  "image/jpeg" },
    { ".au",    "audio/basic" },
    { ".mpeg",  "video/mpeg" },
    { ".mpg",   "video/mpeg" },
    { ".avi",   "video/x-msvideo" },
    { ".gz",    "application/x-gzip" },
    { ".tar",   "application/x-tar" },
    { ".css",   "text/css "},
    { ".js",    "text/javascript "},
};

const std::unordered_map<int, std::string> httpResponse::CODE_STATUS = {
    { 200, "OK" },
    { 400, "Bad Request" },
    { 403, "Forbidden" },
    { 404, "Not Found" },
};

httpResponse::httpResponse()
{
    code = -1;
    path = srcDir = "";
    mmFile = nullptr;
    isKeepAlive = false;
    mmFileStat = { 0 };
}

httpResponse::~httpResponse()
{
    unmapFile();
}

void httpResponse::init(const std::string& srcDir, std::string& path, bool isKeepAlive, int code) {
        assert(srcDir != "");
        if(mmFile) { unmapFile(); }
        this->isKeepAlive = isKeepAlive;
        this->code = code;
        this->path = path;
        this->srcDir = srcDir;
        mmFile = nullptr; 
        mmFileStat = { 0 };
}

void httpResponse::makeResponse(Buffer& buff) {
    /* 判断请求的资源文件 */
    if(stat((srcDir + path).data(), &mmFileStat) < 0 || S_ISDIR(mmFileStat.st_mode)) {
        code = 404;
    }
    else if(!(mmFileStat.st_mode & S_IROTH)) {
        code = 403;
    }
    else if(code == -1) { 
        code = 200; 
    }
    addStateLine(buff);
    addHeader(buff);
    addContent(buff);
}

char* httpResponse::file() {
    return mmFile;
}

size_t httpResponse::fileLen() const {
    return mmFileStat.st_size;
}


void httpResponse::addStateLine(Buffer& buff) {
    std::string status;
    if(CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS.find(code)->second;
    }
    else {
        code = 400;
        status = CODE_STATUS.find(400)->second;
    }
    buff.append("HTTP/1.1 " + std::to_string(code) + " " + status + "\r\n");
}

void httpResponse::addHeader(Buffer& buff) {
    buff.append("Connection: ");
    if(isKeepAlive) {
        buff.append("keep-alive\r\n");
        buff.append("keep-alive: max=6, timeout=120\r\n");
    } else{
        buff.append("close\r\n");
    }
    buff.append("Content-type: " + getFileType() + "\r\n");
}

void httpResponse::addContent(Buffer& buff) {
    int srcFd = open((srcDir + path).data(), O_RDONLY);
    if(srcFd < 0) { 
        errorContent(buff, "File NotFound!");
        return; 
    }

    /* 将文件映射到内存提高文件的访问速度 
        MAP_PRIVATE 建立一个写入时拷贝的私有映射*/
    LOG_DEBUG("file path %s", (srcDir + path).data());
    int* mmRet = (int*)mmap(0, mmFileStat.st_size, PROT_READ, MAP_PRIVATE, srcFd, 0);
    if(*mmRet == -1) {
        errorContent(buff, "File NotFound!");
        return; 
    }
    mmFile = (char*)mmRet;
    close(srcFd);
    buff.append("Content-length: " + std::to_string(mmFileStat.st_size) + "\r\n\r\n");
}

void httpResponse::unmapFile() {
    if(mmFile) {
        munmap(mmFile, mmFileStat.st_size);
        mmFile = nullptr;
    }
}

std::string httpResponse::getFileType() {
    /* 判断文件类型 */
    std::string::size_type idx = path.find_last_of('.');
    if(idx == std::string::npos) {
        return "text/plain";
    }
    std::string suffix = path.substr(idx);
    if(SUFFIX_TYPE.count(suffix) == 1) {
        return SUFFIX_TYPE.find(suffix)->second;
    }
    return "text/plain";
}

void httpResponse::errorContent(Buffer& buff, std::string message) 
{
    std::string body;
    std::string status;
    body += "<html><title>Error</title>";
    body += "<body bgcolor=\"ffffff\">";
    if(CODE_STATUS.count(code) == 1) {
        status = CODE_STATUS.find(code)->second;
    } else {
        status = "Bad Request";
    }
    body += std::to_string(code) + " : " + status  + "\n";
    body += "<p>" + message + "</p>";
    body += "<hr><em>WebServer</em></body></html>";

    buff.append("Content-length: " + std::to_string(body.size()) + "\r\n\r\n");
    buff.append(body);
}