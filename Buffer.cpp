//
// Created by zy on 2021/12/6.
//

#include "Buffer.h"
namespace TinyServer{
ssize_t Buffer::readFd(int fd, int *savedError) {
    // 创建一个临时缓冲区，处理buffer大小不够到情况
    char extraBuf[65536];
    struct iovec vec[2];
    const size_t writable = writableBytes();

    vec[0].iov_base = getBufferBegin() + m_writeIndex;
    vec[0].iov_len = writable;
    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof(extraBuf);

    const ssize_t n = ::readv(fd, vec, 2);

    if(n < 0){
        printf("[Buffer:readFd]fd = %d readv : %s \n", fd, strerror(errno));
        *savedError = errno;
    }else if(static_cast<size_t>(n) < writable){
        m_writeIndex += n;
    }else{
        m_writeIndex = m_buffer.size();
        append(extraBuf, n - writable);
    }

    return n;
}

ssize_t Buffer::writeFd(int fd, int *savedError) {
    size_t  nLeft = readableBytes();
    char* bufPtr = getBufferBegin() + m_readIndex;
    ssize_t n;
    if((n = ::write(fd, bufPtr, nLeft)) <= 0){
        if(n < 0 && n==EINTR)
            return 0;
        else{
            printf("[Buffer:writeFd]fd = %d write : %s\n", fd, strerror(errno));
            *savedError = errno;
            return -1;
        }
    }else{
        m_readIndex += n;
        return n;
    }
    return 0;
}
}