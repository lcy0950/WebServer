// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "AsyncLogging.h"
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <functional>
#include "LogFile.h"

AsyncLogging::AsyncLogging(std::string logFileName_, int flushInterval)
    : flushInterval_(flushInterval),
      running_(false),
      basename_(logFileName_),
      thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging"),
      mutex_(),
      cond_(mutex_),
      currentBuffer_(new Buffer),
      nextBuffer_(new Buffer),
      buffers_(),//一个储存buffer智能指针的数组
      latch_(1) {
  assert(logFileName_.size() > 1);
  currentBuffer_->bzero();//清空缓冲区
  nextBuffer_->bzero();//清空缓冲区
  buffers_.reserve(16);//扩充vector的size
}

//这里需要注意len的长度不能过大
void AsyncLogging::append(const char* logline, int len) {
  MutexLockGuard lock(mutex_);//这部分是临界区,需要独自享有
  if (currentBuffer_->avail() > len)//如果当前的缓冲区够的话,则存入
    currentBuffer_->append(logline, len);
  else {
    buffers_.push_back(currentBuffer_);//注意这是个智能指针
    currentBuffer_.reset();//因为是真能指针的话,这个reset相当于说重开了一片区域
    if (nextBuffer_)
      currentBuffer_ = std::move(nextBuffer_);//减少了一次内存申请的过程，当nextBuffer用掉之后,再申请新的内存
    else
      currentBuffer_.reset(new Buffer);
    currentBuffer_->append(logline, len);
    cond_.notify();//时间到达或者填满缓冲区后
  }
}

void AsyncLogging::threadFunc() {
  assert(running_ == true);
  latch_.countDown();//
  LogFile output(basename_);//输出文件LogFile
  
  BufferPtr newBuffer1(new Buffer);
  BufferPtr newBuffer2(new Buffer);
  newBuffer1->bzero();
  newBuffer2->bzero();
  
  BufferVector buffersToWrite;
  buffersToWrite.reserve(16);
  
    
  while (running_) {
    assert(newBuffer1 && newBuffer1->length() == 0);//保证newBuffer1和2都是全新的
    assert(newBuffer2 && newBuffer2->length() == 0);
    assert(buffersToWrite.empty());

    {
      MutexLockGuard lock(mutex_);
      if (buffers_.empty())  // unusual usage!
      {
        cond_.waitForSeconds(flushInterval_);//这里应该是等两秒钟的意思,可以被提前notify
      }
      buffers_.push_back(currentBuffer_);
      currentBuffer_.reset();//其指向内存并未释放,其跑到buffers里面了,这时,currentBuffer是NULL
        
      //这里的目的是让currentBuffer和nextBuffer处于和刚开始一样的状态
      currentBuffer_ = std::move(newBuffer1);//把newBuffer1的内容投过去
      buffersToWrite.swap(buffers_);//这里把buffersToWrite换掉buffers,即把当前的buffers给写入Log
      if (!nextBuffer_) {//这个nextBuffer_被用掉了,则将其赋值一个新的,这时可以知道的是buffers的大小肯定大于2了
        nextBuffer_ = std::move(newBuffer2);//就把newBuffer2给他
      }
    }
    //这里newBuffer1和newBuffer2都是null了

    assert(!buffersToWrite.empty());

    if (buffersToWrite.size() > 25) {//当buffer大于25个时,这是丢弃一些buffer,这些buffer被丢弃掉
      // char buf[256];
      // snprintf(buf, sizeof buf, "Dropped log messages at %s, %zd larger
      // buffers\n",
      //          Timestamp::now().toFormattedString().c_str(),
      //          buffersToWrite.size()-2);
      // fputs(buf, stderr);
      // output.append(buf, static_cast<int>(strlen(buf)));
      buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
    }

    for (size_t i = 0; i < buffersToWrite.size(); ++i) {
      // FIXME: use unbuffered stdio FILE ? or use ::writev ?
      output.append(buffersToWrite[i]->data(), buffersToWrite[i]->length());//向文件中正式写入了
    }

    if (buffersToWrite.size() > 2) {//保留两个,如果没有两个则其至少有1个
      // drop non-bzero-ed buffers, avoid trashing
      buffersToWrite.resize(2);
    }

    if (!newBuffer1) {//如果newBuffer1是null,则将其复制为buffersToWrite的末尾一个,这个if语句是一定可以执行的
      assert(!buffersToWrite.empty());
      newBuffer1 = buffersToWrite.back();
      buffersToWrite.pop_back();
      newBuffer1->reset();
    }

    if (!newBuffer2) {//如果newBuffer2是null时,说明nextBuffer已经被用掉,newBuffer2已经移动给了nextBuffer
      assert(!buffersToWrite.empty());   
      newBuffer2 = buffersToWrite.back();//这时更新了
      buffersToWrite.pop_back();
      newBuffer2->reset();
    }
    
    assert(true==buffersToWrite.empty());//这里buffersToWrite必为空了。
    buffersToWrite.clear();//清空buffer,其实这句话不用,因为到这了肯定其已经是0了
    output.flush();
  }
  output.flush();
}
