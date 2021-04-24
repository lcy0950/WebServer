// @Author Lin Ya
// @Email xxbbb@vip.qq.com
#include "Server.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <functional>
#include "Util.h"
#include "base/Logging.h"

Server::Server(EventLoop *loop, int threadNum, int port)
    : loop_(loop),
      threadNum_(threadNum),
      eventLoopThreadPool_(new EventLoopThreadPool(loop_, threadNum)),//问题,为什么这个线程池需要 EventLoop的指针?
      started_(false),
        
      //这里的acceptChannel是一个指针
      acceptChannel_(new Channel(loop_)),//问题,为什么这个Chennel需要EventLoop指针?start函数里给出了答案,用于向这个EventLoop进行注册，注册后其被监听
                                         //问题,loop_仅仅是用于被Channel注册嘛？
      port_(port),
      listenFd_(socket_bind_listen(port_)) {
  acceptChannel_->setFd(listenFd_);//设置Fd的属性
  handle_for_sigpipe();//这个函数起什么作用?
  if (setSocketNonBlocking(listenFd_) < 0) {//为什么要设置listenFd为非阻塞的?
    perror("set socket non block failed");
    abort();
  }
}

//可见start函数最重要的部分就是将监听函数放入
void Server::start() {
  eventLoopThreadPool_->start();//线程池开始进行支持
  // acceptChannel_->setEvents(EPOLLIN | EPOLLET | EPOLLONESHOT);
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);//设置监听事件和阻塞方式
  acceptChannel_->setReadHandler(bind(&Server::handNewConn, this));//设置回调函数，this指针绑定这个服务器的,这样这个函数就可以当作一个全局函数使用了。
  
  //void handThisConn() { loop_->updatePoller(acceptChannel_); }这个函数长这样,在server里面这么用,this指针已经绑定loop,最终channel调用这个函数
  //最终其作用相当于channel通过调用connHandler,使得server调用server里面的EventLoop来更新Channel,这样新加的Channel就能被update了
  //问题:acceptChannel可以直接通过loop进行更新,为什么要通过这种方式来弄搞呢？
  //回答,分两个方面来回答,首先hand...是用于回调的,因此其不能有参数,其需要进行一下bind
  //其次,回调函数的调用应该由不同的server进行负责,Channel只做好Channel的事情就好了,不需要越俎代庖的将Server的活
  //包揽了,如果换一个Server,通过现有的设置,Channel可以继续重用,而如果像问题里面的设计,则这个Channel就没办法重用了
  acceptChannel_->setConnHandler(bind(&Server::handThisConn, this));//设置回调函数,this指针绑定这个服务器的。
    
    
  loop_->addToPoller(acceptChannel_, 0);//EventPoll注册监听的Chenel，问题
  started_ = true;//开始标志设置
}

//用于处理新的连接
void Server::handNewConn() {
  struct sockaddr_in client_addr;
  memset(&client_addr, 0, sizeof(struct sockaddr_in));
  socklen_t client_addr_len = sizeof(client_addr);
  int accept_fd = 0;//客户连接的fd
  //处理客户的链接
  while ((accept_fd = accept(listenFd_, (struct sockaddr *)&client_addr,//等有通知了,才执行到这里,问题?这里的listenFd_是阻塞还是非阻塞,为什么这样设置。
                             &client_addr_len)) > 0) {
    EventLoop *loop = eventLoopThreadPool_->getNextLoop();//问题,这个getNextLoop是干什么用的,这里为什么又多了一个loop?
    LOG << "New connection from " << inet_ntoa(client_addr.sin_addr) << ":"//记录客户信息
        << ntohs(client_addr.sin_port);
    // cout << "new connection" << endl;
    // cout << inet_ntoa(client_addr.sin_addr) << endl;
    // cout << ntohs(client_addr.sin_port) << endl;
    /*
    // TCP的保活机制默认是关闭的
    int optval = 0;
    socklen_t len_optval = 4;
    getsockopt(accept_fd, SOL_SOCKET,  SO_KEEPALIVE, &optval, &len_optval);
    cout << "optval ==" << optval << endl;
    */
    // 限制服务器的最大并发连接数
    if (accept_fd >= MAXFDS) {
      close(accept_fd);//为何这里直接close,close接口的语义作用是什么?
      continue;
    }
    // 设为非阻塞模式
    if (setSocketNonBlocking(accept_fd) < 0) {//问题?为什么与客户通信的fd要设置为非阻塞模式.
      LOG << "Set non block failed!";
      // perror("Set non block failed!");
      return;
    }

    setSocketNodelay(accept_fd);//问题,Nodelay和NoLinger参数都是什么含义,用于什么场景?为什么?
    // setSocketNoLinger(accept_fd);
    
      
    //可见,得到这个accept_fd之后,将其交给新的一个loop去搞了
    //这个新loop和HttpData是关键内容
    shared_ptr<HttpData> req_info(new HttpData(loop, accept_fd));//HttpData这个类是用于什么?为何要传入loop和accept_fd,猜想是用于处理http数据的
    req_info->getChannel()->setHolder(req_info);//这里的getChannel是什么语义?setHolder是什么语义
    loop->queueInLoop(std::bind(&HttpData::newEvent, req_info));//queueInLoop是什么作用.
  }
  acceptChannel_->setEvents(EPOLLIN | EPOLLET);//继续监听新的客户。
}
