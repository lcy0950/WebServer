// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "EventLoopThreadPool.h"

EventLoopThreadPool::EventLoopThreadPool(EventLoop *baseLoop, int numThreads)
    : baseLoop_(baseLoop), started_(false), numThreads_(numThreads), next_(0) {
  if (numThreads_ <= 0) {
    LOG << "numThreads_ <= 0";
    abort();
  }
}

void EventLoopThreadPool::start() {
  baseLoop_->assertInLoopThread();
  started_ = true;
  for (int i = 0; i < numThreads_; ++i) {
    std::shared_ptr<EventLoopThread> t(new EventLoopThread());
    threads_.push_back(t);
    loops_.push_back(t->startLoop());//startLoop会生成一个新的EventLoop并且进行Loop,最终不使用的时候会阻塞在eventLoop的Loop上.
  }
}

EventLoop *EventLoopThreadPool::getNextLoop() {
  baseLoop_->assertInLoopThread();//这个函数只能够在主loop中使用
  assert(started_);//表示此线程池一定已经开始工作了。
  EventLoop *loop = baseLoop_;
  if (!loops_.empty()) {//分配个几个Loop线程中的一个进行处理
    loop = loops_[next_];
    next_ = (next_ + 1) % numThreads_;
  }
  return loop;//返回被分配的那个线程,通过这种方式,将应对客户访问的压力进行分摊
}
