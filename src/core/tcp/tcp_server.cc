/*************************************************************************
	> File Name: tcp_server.cc
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时32分42秒
 ************************************************************************/
#ifdef _MSC_VER
#include"tcp_server.h"
#include"acceptor.h"
#include"socket_extension.h"
#include"../reactor/eventloop.h"
#include"../reactor/eventloop_thread_pool.h"
#include"../../dependent/logger/logger.h"
#else
#include"core/tcp/tcp_server.h"
#include"core/tcp/acceptor.h"
#include"core/tcp/socket_extension.h"
#include"core/reactor/eventloop.h"
#include"core/reactor/eventloop_thread_pool.h"
#include"dependent/logger/logger.h"
#endif

#include<limits>

namespace imnet {

namespace core {

//单个线程池默认的IO线程数目
const int kDefaultThreadPoolNumber = 10;

//brief:初始化基于IO线程池的Tcp协议的服务器
//becare:根据环境变量 _IMNET_TCPSERVER_REUSEPORT 确定是否允许重用端口
TcpServer::TcpServer(Eventloop* loop, const string& basename, const Sockaddr& listen_address)
	:__loop(loop),
	__basename(basename),
	__listen_address(listen_address.inet_ntop_withport()),
	__count(0),
	//__is_running(false),
	__is_running(false),
	__acceptor(new Acceptor(__loop,listen_address)),
	__container(),
	__thread_pool(new EventloopThreadPool(__loop,__basename,kDefaultThreadPoolNumber)),
	__on_connection_call(defaultConnectionCallback),
	__on_message_call(defaultMessageCallback){

	//设置acceptor属性
	if (::getenv("IMNET_TCPSERVER_REUSEPORT"))
		__acceptor->setReusePort(true);
	//else
	//	__acceptor->setReusePort(false);

	__acceptor->setConnectionCallback(std::bind(&TcpServer::__newConnectionCallback,
		                                        this,std::placeholders::_1,std::placeholders::_2));
}

//brief:服务端退出清理
//becare:正常情况下会等待子线程执行结束
TcpServer::~TcpServer() {
	__loop->assertInLoopThread();
	LOG_INFO << "TcpServer:" << __basename << "will be destroyed";

	//为当前所有的连接注册清退处理例程
	for (auto& item : __container) {
		TcpConnectionPtr tmp = item.second;
		item.second.reset();
		tmp->getEventloop()->pushBackIntoLoop(std::bind(&TcpConnection::connectionDestroyed, tmp));
	}
}

//brief:设置线程初始化历程，仅在未启动前有效,比如切换工作目录之类的
//becare:慎重inline
//inline void TcpServer::setThreadInitCallback(const ThreadInitCallbackType& callback) {
void TcpServer::setThreadInitCallback(const ThreadInitCallbackType& callback) {
	//__on_thread_init_call = callback;
	__thread_pool->setThreadInitCallback(callback);
}
	

//brief:将fd封装成TcpConnection,
//      本函数被绑定到Acceptor的handleRead()
void TcpServer::__newConnectionCallback(int sockfd, const Sockaddr& peer_address) {
	//TODO:
	__loop->assertInLoopThread();

	char buf[32];
	static_assert(sizeof buf >= std::numeric_limits<CountType>::digits10 + 2, "name buf is not enough.");

	//连接名称
	snprintf(buf, sizeof buf, "#%d", __count);
	++__count;
	string name = __basename + buf;

	//挑选工作线程
	Eventloop* io_loop = __thread_pool->getNextEventloop();
	//获取连接的本地地址，主要是分配的端口号
	Sockaddr local_address(socket_ops::getLocalSockaddrFromFd(sockfd));

	TcpConnectionPtr new_connect = std::make_shared<TcpConnection>(io_loop, name,
	                                    	sockfd, local_address, peer_address);
	LOG_INFO << "TcpServer(" << __basename << ") establish new connection:" << name
		<< "[local:" << local_address.inet_ntop_withport() << ",peer:"
		<< peer_address.inet_ntop_withport() << "]";

#ifndef NODEBUG
	auto item = __container.insert({ name,new_connect });
	if(item.second==false){
		//非常极端状况的处理，理论上不会出现
		LOG_ERROR << "TcpServer(" << __basename << ")establish a repeated connection" << name
			<< " peer:" << peer_address.inet_ntop_withport();
		new_connect.reset();
	}
#else
	__container.insert({ name,new_connect });
#endif
	
	//设置新连接的属性
	new_connect->setConnectionCallback(__on_connection_call);
	new_connect->setMessageCallback(__on_message_call);
	new_connect->setCloseCallback(std::bind(&TcpServer::__removerConnectionCallback, this, std::placeholders::_1));
	new_connect->setWriteCompleteCallback(__on_write_complete_call);

	//完成本连接
	//becare:只能在io线程中完成
	//__loop->pushBackIntoLoop(std::bind(&TcpConnection::connectionEstablished, new_connect));
	io_loop->pushBackIntoLoop(std::bind(&TcpConnection::connectionEstablished, new_connect));
}

//brief:清理在Server线程中的数据，根据close流程之后还会注册TcpConnection的connectionDestroyed任务
//becare:注意务必防止对象析构早于其对象成员的调用，因此最后一道清理例程必须要离开本次loop(或者在队列中唤醒)后执行
void TcpServer::__removerConnectionCallback(const TcpConnectionPtr& connection) {
	__loop->pushBackIntoLoop(std::bind(&TcpServer::__removeConnectionCallbackInloop, this, connection));
}
void TcpServer::__removeConnectionCallbackInloop(const TcpConnectionPtr& connection) {
	__loop->assertInLoopThread();

	LOG_INFO << "TcpServer(" << __basename << ")remove: " << connection->getName();
	auto num = __container.erase(connection->getName());
	assert(num == 1); (void)num;

	//清理对应io线程的数据
	Eventloop* ioloop = connection->getEventloop();
	//FIXME:必须要加入到队列中
	//ioloop->pushBackIntoLoop(std::bind(&TcpConnection::connectionDestroyed, connection));
	ioloop->pushBackIntoQueue(std::bind(&TcpConnection::connectionDestroyed, connection));
}

//brief:设置线程数量
//becare:未启动之前有效
void TcpServer::setThreadNumber(size_t number) {
	if (__is_running.load() == 0) {
		__thread_pool->setThreadNumber(number);
	}
	//else {
	//	LOG_WARN << "TcpServer:" << __basename << " has been running";
	//}
}

//brief:启动IO线程,包括启动并初始化IO线程，然后启动监听
//becare:未启动之前有效
void TcpServer::start() {
	if (0 == __is_running.fetch_add(1)) {
		//首次调用有效
		__thread_pool->start();

		assert(false == __acceptor->isListening());
		__loop->pushBackIntoLoop(std::bind(&Acceptor::listen, __acceptor.get()));
	}
	//else {
	//	LOG_WARN << "TcpServer:" << __basename << " has been running";
	//}
}

}//!namespace core
}//!namespace imnet
