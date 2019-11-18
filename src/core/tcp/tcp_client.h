/*************************************************************************
	> File Name: tcp_client.h
	> Author: Dream9
	> Brief: 
	> Created Time: 2019年10月10日 星期四 10时33分42秒
 ************************************************************************/
#ifndef _CORE_TCP_CLENT_H_
#define _CORE_TCP_CLENT_H_

#ifdef _MSC_VER
#include"sockaddr.h"
#include"callback_types.h"
#include"../../dependent/advanced/mutexlock.h"
#include"../../dependent/advanced/atomic_type.h"
#else
#include"core/tcp/sockaddr.h"
#include"core/tcp/callback_types.h"
#include"dependent/advanced/mutexlock.h"
#include"dependent/advanced/atomic_type.h"
#endif

#include<memory>
#include<functional>

namespace imnet {

namespace core {

class Eventloop;
class Connector;

class TcpClient : noncopyable {
public:
	TcpClient(Eventloop* loop,const Sockaddr& address, const string& name);
	~TcpClient();

	void connect();//尝试连接
	void stopconnect();//暂停尝试
	void disconnect();//断开连接
	//TODO:
	void redirect(const Sockaddr& new_address);//重定向

	//brief:设置断开重连
	void activateRetry() {
		__need_retry.store(true);
	}
	void deactivateRetry() {
		__need_retry.store(false);
	}
	bool isRetry() {
		return __need_retry.load();
	}

	//设置用户可注册回调
	void setConnectionCallback(const ConnectionCallbackType& call) {
		__on_connect = call;
	}
	void setMessageCallback(const MessageCallbackType& call) {
		__on_message = call;
	}
	void setWriteCompleteCallback(const WriteCompleteCallbackType& call) {
		__on_write_complete = call;
	}

private:
	//Connector响应连接建立的回调
	void __newConnection(int sockfd);
	//TcpConnection的关闭例程
	void __removeConnection(const TcpConnectionPtr& connect);

	Eventloop* __loop;
	string __basename;
	Sockaddr __server;//对方服务端地址
	int __count;//本地已建立连接计数
	//FIXME:为了延缓Connector的析构，必要时要把该句柄传出去
	//std::unique_ptr<Connector> __connector;
	std::shared_ptr<Connector> __connector;

	//以下变量可能会被用户使用或修改（通过暴露的接口）
	detail::Atomictype<bool> __need_connect;
	detail::Atomictype<bool> __need_retry;
	TcpConnectionPtr __connection;
	Mutex __mutex;//用于保护__connection
	
	//回调函数，类比TcpServer
	//becare: __on_message和__on_connect总是有一份默认实现
	ConnectionCallbackType __on_connect;
	MessageCallbackType __on_message;

	WriteCompleteCallbackType __on_write_complete;
	//HighWaterCallbackType __on_high_water;

};
}
}

#endif
