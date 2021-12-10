//
// Created by zhou on 2021/12/7.
//

#ifndef TINYSERVER_HTTPREQUEST_H
#define TINYSERVER_HTTPREQUEST_H
#include "all.h"
#include "Buffer.h"
#define STATIC_ROOT "../www"

namespace TinyServer{
class TimerNode;

class HttpRequest {
public:
    // 报文解析状态
    enum HttpRequestParseState{
        RequestLine,
        RequestHeader,
        RequestBody,
        GetAll
    };
    // Http请求方法
    enum HttpMethod{
        Invalid,
        Get,
        Post,
        Head,
        Put,
        Delete
    };
    // Http版本
    enum HttpVersion{
        Unknown,
        Http1_0,
        Http1_1
    };

private:
    int m_fd;
    Buffer m_inBuffer;      // 读缓冲区
    Buffer m_outBuffer;     // 写缓冲区
    bool m_isWorking;       // 若正在工作，则不能被超时事件断开

    TimerNode* m_timer;     // 定时器

    // 报文解析
    HttpRequestParseState m_state;                  // 报文解析状态
    HttpMethod m_method;                            // HTTP方法
    HttpVersion m_version;                          // HTTP版本
    std::string m_path;                             // URL路径
    std::string m_queryPara;                        // URL参数
    std::map<std::string, std::string> m_header;    // 报文头部

public:
    HttpRequest(int fd);
    ~HttpRequest();

    int getFd(){
        return m_fd;
    }

    int readData(int *savedErrno);
    int writeData(int *savedErrno);

    void appendOutBuffer(const Buffer &buf){
        m_outBuffer.append(buf);
    }

    int writableBytes(){
        return m_outBuffer.readableBytes();
    }

    void setTimer(TimerNode * time){
        m_timer = time;
    }

    TimerNode* getTimer(){
        return m_timer;
    }

    void setWorking(){
        m_isWorking = true;
    }

    void setNoWorking(){
        m_isWorking = false;
    }

    bool isWorking() const{
        return m_isWorking;
    }

    bool parseRequest();    // 解析报文
    bool parseFinish(){
        return m_state == GetAll;
    }

    void resetParse();  // 重置解析状态

    std::string getPath()const{
        return m_path;
    }

    std::string getQuery() const{
        return m_queryPara;
    }

    std::string getHeader(const std::string &field) const;
    std::string getMethod() const;

    bool keepAlive() const;

private:
    bool parseRequestLine(const char* begin, const char* end);
    bool setHttpMethod(const char* begin, const char* end);

    void setPath(const char* begin, const char* end){
        std::string subPath;
        subPath.assign(begin, end);
        if(subPath == "/")
            subPath = "/index.html";
        m_path = STATIC_ROOT + subPath;
    }

    void setQueryParameter(const char* begin, const char* end){
        m_queryPara.assign(begin, end);
    }

    void setHttpVersion(HttpVersion version){
        m_version = version;
    }

    void addHeader(const char* begin, const char* colon , const char* end);
};

}
#endif //TINYSERVER_HTTPREQUEST_H
