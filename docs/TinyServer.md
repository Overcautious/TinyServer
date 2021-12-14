**模型架构**

### 1. 初始化
- 绑定地址和端口，创建非阻塞监听描述符
- 创建Epoll实例
- 创建线程池
- 创建定时器管理器
```c++
Server::Server(u_short port, int numThread)
        :m_port(port), m_lsnFd(Utils::createListenFd(m_port)),
        m_lsnReq(static_cast<HttpRequest *>(new HttpRequest(m_lsnFd))), m_epoll(new Epoll()),
        m_threadPool(new ThreadPool(numThread)), m_timer(new Timer()){
    assert(m_lsnFd >= 0);
}
```

### 2. 运行服务器
- 注册监听文件描述符的可读事件
- 注册新连接、可读事件、可写事件和关闭连接的事件处理函数（回调）
- 进入事件循环
> - 通过定时器管理器获取下一个超时时间点与现在的时间差timeDiff
> - 设置超时时间`timeDiff`，调用`m_epoll->wait()`，阻塞等待监听事件发生
> - 调用`m_epoll->wait()`，分配事件处理函数，新连接处理函数和断开连接处理函数在I/O线程调用，连接描述符的读写事件处理函数在线程池调用
> - 处理超时事件

```C++
void TinyServer::run()
{
    m_epoll->add(m_lsnFd, m_lsnReq.get(), (EPOLLIN | EPOLLET));
    m_epoll->setConnectionCallbak(std::bind(&Server::acceptConnection, this));
    m_epoll->setCloseConnectionCallbak(std::bind(&Server::closeConnection, this, std::placeholders::_1));
    m_epoll->setRequest(std::bind(&Server::doRequest, this, std::placeholders::_1));
    m_epoll->setResponse(std::bind(&Server::doResponse, this, std::placeholders::_1));

    while (1)
    {
        int timeDiff = m_timerManager->getNearestExpiredTimer();
        int eventsNum = m_epoll->wait(timeDiff);
        if (eventsNum > 0)
        {
            m_epoll->handleEvent(m_lsnFd, m_threadPool, eventsNum);
        }
        m_timerManager->handleExpiredTimers();
    }
}
```

### 3. 接受连接
- 监听文件描述符设置为ET模式，所以需要循环读监听描述符直到返回EAGAIN错误
- 调用`accept4`接受新连接，`accept4`函数可以直接设置新连接描述符为非阻塞模式
- 为新连接分配一个`HttpRequest对象`，设置定时器和注册可读事件到`m_epoll`
```C++
void Server::acceptConnection()
{
    while (1)
    {
        int acceptFd = ::accept4(m_lsnFd, nullptr, nullptr, SOCK_NONBLOCK | SOCK_CLOEXEC);
        if (acceptFd == -1)
        {
            if (errno == EAGAIN) break;
            printf("[TinyServer::acceptConnection] accept: %s\n", strerror(errno));
            break;
        }

        HttpRequest* req = new HttpRequest(acceptFd);
        m_timerManager->addTimer(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        m_epoll->add(acceptFd, req, (EPOLLIN | EPOLLONESHOT));
    }
}
```

### 4. 关闭连接
- 判断该连接是否活跃(`working`)，若活跃则返回，不断开连接
- 若不活跃，删除定时器，从epoll的监听描述符中删除该文件描述符，回收HttpRequest对象`req`，断开连接
```C++
void Server::closeConnection(HttpRequest *req)
{
    int fd = req->getFd();
    if (req->isWorking()) return;

    m_timerManager->delTimer(req);
    m_epoll->del(fd, req, 0);
    delete req;
    req = nullptr;
}
```

### 5. 可读事件
- 删除文件描述符的超时定时器
- 从文件描述符中读数据，根据read的返回值处理
> - 返回0，断开连接
> - EAGAIN错误，对文件描述符进行epoll的MOD操作，注册可读事件（因为使用了EPOLLONESHOT），设置超时定时器并返回
> - 其它错误，断开连接
> - 返回值大于0，解析报文，若解析报文出错，则返回400响应报文并断开连接
> - 若解析报文完成，则通过HttpResponse类构造响应报文，并注册文件描述符可写事件（使用了LT模式）
> - 解析报文未完成，对文件描述符进行epoll的MOD操作，注册可读事件（因为使用了EPOLLONESHOT）
```C++
void Server::doRequest(HttpRequest *req)
{
    m_timerManager->delTimer(req); // ?
    assert(req != nullptr);
    int fd = req->getFd();

    int rdErrno;
    int retRead = req->readData(&rdErrno);

    if (retRead == 0)
    {
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if (retRead < 0 && (rdErrno != EAGAIN))
    {
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if (retRead < 0 && rdErrno == EAGAIN)
    {
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLONESHOT));
        req->setNoWorking();
        m_timerManager->addTimer(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        return;
    }

    if (!req->parseRequest())
    {
        HttpResponse response(400, "", false);
        req->appendOutBuffer(response.makeResponse());
        int wrErrno;
        req->writeData(&wrErrno);
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if (req->parseFinish())
    {
        HttpResponse response(200, req->getPath(), req->keepAlive());
        req->appendOutBuffer(response.makeResponse());
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLOUT | EPOLLONESHOT)); //?
    }
}
```

### 6. 可写事件
- 删除文件描述符的超时定时器
- 若文件描述符的输出缓冲区为空，设置超时定时器，直接返回
- 往文件描述符中写数据，根据write的返回值处理
> - EAGAIN错误，对文件描述符进行epoll的MOD操作，注册可写事件（因为使用了EPOLLONESHOT），返回
> - 其它错误，断开连接
> - 缓冲区的数据写完，如果是HTTP长连接，重置HTTP解析状态，对文件描述符进行epoll的MOD操作，注册可读事件（因为使用了EPOLLONESHOT），设置超时定时器，返回；不是HTTP长连接则断开连接
> - 缓冲区的数据没有写完，对文件描述符进行epoll的MOD操作，注册可读事件（因为使用了EPOLLONESHOT），设置超时定时器，返回
```C++
void Server::doResponse(HttpRequest *req)
{
    m_timerManager->delTimer(req);
    assert(req != nullptr);
    int fd = req->getFd();
    int bytesToWrite = req->writableBytes();
    if (bytesToWrite == 0)
    {
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLONESHOT));
        req->setNoWorking();
        m_timerManager->addTimer(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        return;
    }

    int wrErrno;
    int retWrite = req->writeData(&wrErrno);

    if (retWrite < 0 && wrErrno == EAGAIN)
    {
        m_epoll->mod(fd, req, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
        return;
    }

    if (retWrite < 0 && (wrErrno != EAGAIN))
    {
        req->setNoWorking();
        closeConnection(req);
        return;
    }

    if (retWrite == bytesToWrite)
    {
        if (req->keepAlive())
        {
            req->resetParse();
            m_epoll->mod(fd, req, (EPOLLIN | EPOLLONESHOT));
            req->setNoWorking();
            m_timerManager->addTimer(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
        }
        else
        {
            req->setNoWorking();
            closeConnection(req);
        }

        return;
    }

    m_epoll->mod(fd, req, (EPOLLIN | EPOLLOUT | EPOLLONESHOT));
    req->setNoWorking();
    m_timerManager->addTimer(req, CONNECT_TIMEOUT, std::bind(&Server::closeConnection, this, req));
    return;
}
```

### 7. 线程池
- 线程池的定义如下
```C++
class ThreadPool
{
public:
    using JobFunction = std::function<void()>;

private:
    std::vector<std::thread> m_threads;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::queue<JobFunction> m_jobs;
    bool m_isStop;

public:
    ThreadPool(int numWkrs);

    ~ThreadPool();

    void pushJob(const JobFunction &job);
};
```

- 对线程池的添加任务流程如下
> - 对互斥量加锁
> - 把任务push进任务队列
> - 解锁
> - 通过condition_variable唤醒一个阻塞线程

```C++
void ThreadPool::pushJob(const JobFunction &job)
{
    {
        std::unique_lock<std::mutex> lck(m_mtx);
        m_jobs.push(job);
    }

    printf("[ThreadPool::pushJob] push new job\n");
    m_cv.notify_one();
}
```

### 8. 定时器
- 定时器有两个类`TimerNode`和`TimerManager`
- `Timer类`用小根堆管理TimerNode，根据超时时间排序
- `Timer`的关键函数是`addTimer`、`delTimer`、`handleExpireTimers`和`getNextExpireTime`
```C++
void Timer::addTimerNode(HttpRequest *req, const int &time_out, const TimeoutCallback &cb) {
std::unique_lock<std::mutex> lck(m_mtx);
assert(req != nullptr);

updateTime();
TimerNode* time = new TimerNode(m_nowTime + MS(time_out), cb);
m_timerQueue.push(time);

//如果对于同一个request连续调用两次addtimer，则需要删除前一个定时器
if(req->getTimer() != nullptr){
delTimerNode(req);
}
req->setTimer(time);
}

void Timer::delTimerNode(HttpRequest *req) {
assert(req != nullptr);

TimerNode * time = req->getTimer();
if(time == nullptr)
return ;
time->del();    // 如果直接删除，会使最大堆里priority_queue里的指针变成垂悬指针。正确的方法就是惰性删除

req->setTimer(nullptr);
}


void Timer::handleExpiredTimerNode() {
std::unique_lock<std::mutex> lck(m_mtx);    //上锁
updateTime();

while(!m_timerQueue.empty()){
TimerNode* time = m_timerQueue.top();
assert(time != nullptr);

if(time->isDeleted()){
std::cout<<"[Timer::handleExpiredTimerNode] timer= "
<< Clock::to_time_t(time->getExpiredTime()) << " is deleted" <<std::endl;
m_timerQueue.pop();
delete time;
continue;
}
// 队列头部的都没有超时
if((std::chrono::duration_cast<MS>(time->getExpiredTime() - m_nowTime )).count() > 0){
std::cout<< "[Timer::handleExpiredTimerNode] there is no timeout TimerNode" << std::endl;
return;
}
// 超时
std::cout<< "[Timer::handleExpiredTimerNode] one TimerNode timeout" << std::endl;
time->runCallBack();
m_timerQueue.pop();
delete time;

}
}

int Timer::getNearestExpiredTimerNode() {
std::unique_lock<std::mutex> lck(m_mtx);
updateTime();
int res = -1;
while(!m_timerQueue.empty()){
TimerNode * time = m_timerQueue.top();
if(time->isDeleted()){
m_timerQueue.pop();
delete time;
continue;
}
res = std::chrono::duration_cast<MS>(time->getExpiredTime() - m_nowTime ).count();
res = res<0 ? 0:res;
return res;
}

return res;
}
```


