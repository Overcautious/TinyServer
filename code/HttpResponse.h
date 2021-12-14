//
// Created by zhou on 2021/12/10.
//

#ifndef TINYSERVER_HTTPRESPONSE_H
#define TINYSERVER_HTTPRESPONSE_H
#include "all.h"

#define CONNECT_TIMEOUT 500
namespace TinyServer{
class Buffer;
class HttpResponse {
private:
    std::string getFileType();
public:
    HttpResponse(int statusCode, std::string path, bool keepAlive):m_statusCode(statusCode), m_resourcePath(path),
                    m_isKeepAlive(keepAlive){}
    ~HttpResponse(){}

    Buffer makeResponse();

    void doErrorResponse(Buffer& output, std::string msg);
    void doStaticRequest(Buffer& output, long fileSize);

private:
    static const std::map<int, std::string> statuesCode2Msg;
    static const std::map<std::string, std::string> suffix2Type;

    std::map<std::string, std::string> m_headers;
    std::string m_resourcePath;
    int m_statusCode;
    bool m_isKeepAlive;
};

}
#endif //TINYSERVER_HTTPRESPONSE_H
