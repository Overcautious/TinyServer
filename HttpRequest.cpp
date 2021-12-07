//
// Created by zhou on 2021/12/7.
//

#include "HttpRequest.h"

HttpRequest::HttpRequest(int fd): m_fd(fd), m_isWorking(false), m_timer(nullptr),
                            m_state(RequestLine), m_method(Invalid), m_version(Unknown)
{
    assert(fd >= 0);
}

HttpRequest::~HttpRequest() {
    close(m_fd);
}

int HttpRequest::readData(int *savedErrno) {
    int ret = m_inBuffer.readFd(m_fd, savedErrno);
    return ret;
}

int HttpRequest::writeData(int *savedErrno) {
    int ret = m_outBuffer.writeFd(m_fd, savedErrno);
    return ret;
}

bool HttpRequest::parseRequest() {
    bool ok = true;
    bool hasMore = true;

    while(hasMore){
        if(m_state == RequestLine){
            // 请求处理行
            const char* crlf = m_inBuffer.findCRLF();
            if(crlf){
                ok = parseRequestLine(m_inBuffer.peek(), crlf);
                if(ok){
                    m_inBuffer.retrieveUnitil(crlf + 2);
                    m_state = RequestHeader;
                }else{
                    hasMore = false;
                }
            }else{
                hasMore = false;
            }
        }else if(m_state == RequestHeader){
            const char* crlf = m_inBuffer.findCRLF();
            if(crlf){
                const char* colon = std::find(m_inBuffer.peek(), crlf, ':');
                if(colon != crlf){
                    addHeader(m_inBuffer.peek(), colon, crlf);
                }else{
                    m_state = GetAll;
                    hasMore = false;
                }
                m_inBuffer.retrieveUnitil(crlf+2);
            }else{
                hasMore = false;
            }
        }else if(m_state == RequestBody){

        }
    }

    return ok;
}

bool HttpRequest::setHttpMethod(const char *begin, const char *end) {
    std::string m(begin, end);

    if(m == "GET")
        m_method = Get;
    else if(m == "POST")
        m_method = Post;
    else if(m == "HEAD")
        m_method = Head;
    else if(m == "PUT")
        m_method = Put;
    else if(m == "DELETE")
        m_method = Delete;
    else
        m_method = Invalid;

    return m_method == Invalid;
}

bool HttpRequest::parseRequestLine(const char *begin, const char *end) {

    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    if(space != end && setHttpMethod(start, space) ){
        start = space + 1;
        space = std::find(start , end, ' ');
        if(space != end){
            const char* question = std::find(start, space, '?');
            if(question != space){
                setPath(start, question);
                setQueryParameter(question, space);
            }else{
                setPath(start, space);
            }
            start = space + 1;
            succeed = (end - start == 8) && std::equal(start, end-1, "HTTP/1.");

            if(succeed){
                if(*(end - 1) == '1'){
                    setHttpVersion(Http1_1);
                }else if(*(end - 1) == '0'){
                    setHttpVersion(Http1_0);
                }else{
                    succeed = false;
                }
            }
        }
    }

    return succeed;
}

void HttpRequest::addHeader(const char *begin, const char *colon, const char *end) {
    std::string field(begin, colon);
    ++colon;
    while(colon < end && *colon == ' ')
        ++colon;

    std::string value(colon, end);
    while( !value.empty() && value[value.size()-1] == ' ')
        value.resize(value.size() - 1);

    m_header[field] = value;
}

void HttpRequest::resetParse() {
    m_state = RequestLine;
    m_method = Invalid;
    m_version = Unknown;
    m_path = "";
    m_queryPara = "";
    m_header.clear();
}

std::string HttpRequest::getHeader(const std::string &field) const {
    std::string res;

    auto iter = m_header.find(field);
    if(iter != m_header.end()){
        res = iter->second;
    }

    return res;
}

std::string HttpRequest::getMethod() const {
    std::string res;

    if(m_method == Get)
        res = "GET";
    else if(m_method == Post)
        res = "POST";
    else if(m_method == Head)
        res = "HEAD";
    else if(m_method == Put)
        res = "PUT";
    else if(m_method == Delete)
        res = "DELETE";

    return res;
}

bool HttpRequest::keepAlive() const {
    std::string connection = getHeader("Connection");
    bool res = connection == "Keep-Alive" ||
            (m_version == Http1_1 && connection != "close");
    return res;
}



