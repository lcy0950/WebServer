// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThread.h"
#include <functional>

EventLoopThread::EventLoopThread()
    : loop_(NULL),//loop为NULL，loop何时不为null,从哪生成?
      exiting_(false),
      thread_(bind(&EventLoopThread::threadFunc, this), "EventLoopThread"),//绑定函数
      mutex_(),//初始化条件变量
      cond_(mutex_) {}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;//为什么这里的exiting赋值为true,表示已经使用过了嘛?
  if (loop_ != NULL) {
    loop_->quit();
    thread_.join();
  }
}

EventLoop* EventLoopThread::startLoop() {
  assert(!thread_.started());
  thread_.start();//猜想thread会设置一个loop
  {
    MutexLockGuard lock(mutex_);//这里加锁
    // 一直等到threadFun在Thread里真正跑起来,这里需要thread真正的start并且生成一个EventLoop之后
    while (loop_ == NULL) cond_.wait();//等待唤醒,唤醒的条件是loop被设置了
  }
  return loop_;//返回loop
}

void EventLoopThread::threadFunc() {
  EventLoop loop;//生成一个EventLoop

  {
    MutexLockGuard lock(mutex_);
    loop_ = &loop;
    cond_.notify();
  }

  loop.loop();
  // assert(exiting_);
  loop_ = NULL;
}
