## 1. 总体架构
- 采用了同步非阻塞IO模型Reactor
- 采用了事件驱动模型，基于Epoll实现，通过时间回调的方式实现业务逻辑
- 采用了线程池，避免了线程的频繁创建和销毁
- 定时器处理非活动连接，由小根堆实现，关闭超时的非活动连接
- 自定义Buffer类，实现自动增长的数据缓冲区


## 2. 使用到的C++11内容
- 智能指针shared_ptr
- 智能指针unique_ptr
- 锁unique_lock  
- 使用std::function来作为回调函数
- 使用std::function + std::bind绑定的方式实现回调


