//
// Created by zy on 2021/12/6.
//

#ifndef TINYSERVER_BUFFER_H
#define TINYSERVER_BUFFER_H
#include "all.h"

class Buffer {
private:
    std::vector<char> m_buffer;
    size_t m_readIndex;
    size_t m_writeIndex;
};


#endif //TINYSERVER_BUFFER_H
