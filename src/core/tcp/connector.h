/*************************************************************************
	> File Name: connector.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月29日 星期二 15时08分17秒
 ************************************************************************/
#ifndef _CORE_CONNECTOR_H_
#define _CORe_CONNECTOR_H_

#ifdef _MSC_VER
#include"sockaddr.h"
#include"../../dependent/noncopyable.h"
#else
#include"core/tcp/sockaddr.h"
#include"dependent/noncopyable.h"
#endif

#include<functional>
#include<memory>
#include<atomic>

namespace imnet {

namespace core {

class Eventloop;
class Channel;
class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr; 

//brief:本类负责向指定地址建立连接，并将建立的fd通过注册的ConnectionCallback将其传递给客户端，
//     通常由TcpClient负责该fd的后期维护，本类主要负责应对connect过程中的异常处理和超时重试
//becare:本类是可以重复使用的，即restart建立的连接是不一样的(改变本地随机端口)
class Connector :noncopyable ,public std::enable_shared_from_this<Connector>{
public:
	typedef std::function<void(int sockfd)> ConnectionCallbackType;

	Connector(Eventloop* __loop, const Sockaddr& address);
	~Connector();

	//用户要求与指定地址建立连接
	void start();
	void restartInLoop();//本函数负责客户端的自动重连工作
	void stop();
	void stop(const ConnectorPtr& self);//为了延缓析构

	const Sockaddr& getServerAddress()const {
		return __serve_address;
	}

	bool isDisconnected()const {
		return __state == kDisconnected;
	}

	//beacre:本函数不期望被用户直接使用
	void setConnectionCallback(const ConnectionCallbackType& call) {
		__on_connect = call;
	}

private:
	//brief:本类的状态迁移为：初始kDisconnected,
	//     调用start(首次)/restart,状态为kConnecting,
	//     连接完成并__on_connect之后为kConnected
	//     只有restart可以重复修改kConnecting
	enum State{
		kDisconnected,
		kConnecting,
		kConnected
	};

	void __startInLoop();
	void __stopInLoop();
	void __stopInLoop(const ConnectorPtr& self);//为了延缓析构

	void __connect();
	void __connecting(int sockfd);
	void __retryConnect(int sockfd);

	void handleError();
	void handleWrite();

	int __getFdAndRestChannel();
	void __resetChannelNextLoop();

	void __setState(State state) {
		__state = state;
	}

	Eventloop* __loop;
	Sockaddr __serve_address;
	State __state;//对象状态
	int __backoff;//退避补偿
	//bool __need_connecting;//用户是否需要连接
	std::atomic_bool __need_connecting;//用户是否需要连接,该变量可以不在IO线程中修改
	std::unique_ptr<Channel> __channel;

	ConnectionCallbackType __on_connect;

	static const int kMaxBackoff;
	static const int kInitBackoff;
};

}//!namespace core
}//!namespace imnet




#endif
