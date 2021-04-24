// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <memory>
#include "Channel.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"

class Server {
 public:
  Server(EventLoop *loop, int threadNum, int port);
  ~Server() {}
  EventLoop *getLoop() const { return loop_; }//得到EventLoop对象
  void start();//开始
  void handNewConn();//处理新的连接. question:新连接在哪里存放?
  void handThisConn() { loop_->updatePoller(acceptChannel_); }//这个函数起什么作用?

 private:
  EventLoop *loop_;
  int threadNum_;
  std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;//存放线程池的内容
  bool started_;
  std::shared_ptr<Channel> acceptChannel_;//用于监听的Channel
  int port_;
  int listenFd_;//监听的文件描述符
  static const int MAXFDS = 100000;
};
