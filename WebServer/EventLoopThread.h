// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include "EventLoop.h"
#include "base/Condition.h"
#include "base/MutexLock.h"
#include "base/Thread.h"
#include "base/noncopyable.h"


class EventLoopThread : noncopyable {
 public:
  EventLoopThread();
  ~EventLoopThread();
  EventLoop* startLoop();//开始轮询

 private:
  void threadFunc();//一个函数,他是用来干什么的？
  EventLoop* loop_;//一个EventLoop,推测是从startLoop处来的,难道有多个线程在Loop?主线程只做accept，其余线程分摊处理连接嘛?
  bool exiting_;//这个是用来干甚么的?
  Thread thread_;//这个是用来干啥的?
  MutexLock mutex_;//所和条件变量
  Condition cond_;
};
