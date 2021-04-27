// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>  
#include <sys/time.h> 

//本文件总结：
//总结一下LOG的使用方法
//LOG首先会建立一个Logger对象,对象的构造函数和<<符号重载使得能够将需要记录的内容放入LogStream中
//然后此对象析构的时候,会将LogStream中储存的内容通过Asyn...Logger进行Append

//下一步就是看看Asyn..是怎样start和append的

static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
static AsyncLogging *AsyncLogger_;

std::string Logger::logFileName_ = "./WebServer.log";

void once_init()
{
    AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());//得到日志文件地址
    AsyncLogger_->start(); //开始工作
}

void output(const char* msg, int len)
{
    pthread_once(&once_control_, once_init);//一次初始化
    AsyncLogger_->append(msg, len);//向AsyncLogger里面写内容
}

Logger::Impl::Impl(const char *fileName, int line)
  : stream_(),
    line_(line),
    basename_(fileName)
{
    formatTime();
}

void Logger::Impl::formatTime()
{
    struct timeval tv;
    time_t time;
    char str_t[26] = {0};
    gettimeofday (&tv, NULL);
    time = tv.tv_sec;
    struct tm* p_time = localtime(&time);   
    strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
    stream_ << str_t;//标明发生时间
}

Logger::Logger(const char *fileName, int line)
  : impl_(fileName, line)
{ }

Logger::~Logger()//这里的析构函数 将LogStream 和 AsyncLogging联系到一起
{
    impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';//赋值构造函数
    const LogStream::Buffer& buf(stream().buffer());//Buffer类型的
    output(buf.data(), buf.length());//向异步Log组件写内容.
}
