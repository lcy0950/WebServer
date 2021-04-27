// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#pragma once
#include <functional>
#include <string>
#include <vector>
#include "CountDownLatch.h"
#include "LogStream.h"
#include "MutexLock.h"
#include "Thread.h"
#include "noncopyable.h"
//文件总结：
//文件是Async...的大致组成
//其中包括有一下几个问题需要继续弄清楚
//1.线程的主要作用,就是thread_变量,那个threadFunc是thread要执行的函数嘛?
//2.lanch是什么意思,CountDownLatch有什么作用
//3.append函数是如何工作的

class AsyncLogging : noncopyable {
 public:
  AsyncLogging(const std::string basename, int flushInterval = 2);
  
  ~AsyncLogging() {
    if (running_) stop();
  }
  
  void append(const char* logline, int len);

  void start() {
    running_ = true;
    thread_.start();//线程开始
    latch_.wait();
  }

  void stop() {
    running_ = false;
    cond_.notify();
    thread_.join();//线程结束
  }

 private:
  void threadFunc();
  typedef FixedBuffer<kLargeBuffer> Buffer;//大容量buffer
  typedef std::vector<std::shared_ptr<Buffer>> BufferVector;//buffer数组
  typedef std::shared_ptr<Buffer> BufferPtr;//buffer指针
  
  const int flushInterval_;//刷新间隔
  bool running_;//是否在运行
  std::string basename_;//这里的basename是.log文件的地址
  Thread thread_;//线程
  
  MutexLock mutex_;//配合使用用于同步
  Condition cond_;
  
  BufferPtr currentBuffer_;//当前buffer
  BufferPtr nextBuffer_;//下一个buffer
  BufferVector buffers_;//buffer数组
  CountDownLatch latch_;//不知是啥
};
